/***************************************************************************
 * SPDX-FileCopyrightText: 2014-2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 * SPDX-FileCopyrightText: 2014 Martin Bříza <mbriza@redhat.com>
 * SPDX-FileCopyrightText: 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ***************************************************************************/

#include "Display.h"

#include "Configuration.h"
#include "DaemonApp.h"
#include "DisplayManager.h"
#include "Greeter.h"
#include "Seat.h"
#include "SessionRunner.h"
#include "SocketServer.h"
#include "Utils.h"

#include <QDebug>
#include <QFile>
#include <QLocalSocket>
#include <QTimer>

#include <pwd.h>
#include <sys/time.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

#include <KConfigGroup>
#include <KDesktopFile>

#include "Login1Manager.h"
#include "Login1Session.h"
#include "VirtualTerminal.h"
#include "config.h"

static int s_ttyFailures = 0;

namespace PLASMALOGIN
{
bool isTtyInUse(const QString &desiredTty)
{
    if (Logind::isAvailable()) {
        OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
        auto reply = manager.ListSessions();
        reply.waitForFinished();

        const auto info = reply.value();
        for (const SessionInfo &s : info) {
            OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), s.sessionPath.path(), QDBusConnection::systemBus());
            if (desiredTty == session.tTY() && session.state() != QLatin1String("closing")) {
                qDebug() << "tty" << desiredTty << "already in use by" << session.user().path.path() << session.state() << session.display()
                         << session.desktop() << session.vTNr();
                return true;
            }
        }
    }
    return false;
}

int fetchAvailableVt()
{
    if (!isTtyInUse(QStringLiteral("tty%1").arg(PLASMALOGIN_INITIAL_VT))) {
        return PLASMALOGIN_INITIAL_VT;
    }
    const auto vt = VirtualTerminal::currentVt();
    if (vt > 0 && !isTtyInUse(QStringLiteral("tty%1").arg(vt))) {
        return vt;
    }
    return VirtualTerminal::setUpNewVt();
}

Display::Display(Seat *parent)
    : QObject(parent)
    , m_auth(new Auth(this))
    , m_seat(parent)
    , m_socketServer(new SocketServer(this))
    , m_greeter(new Greeter(this))
{
    if (seat()->canTTY()) {
        m_terminalId = fetchAvailableVt();
    }
    qDebug("Using VT %d", m_terminalId);

    // respond to authentication requests
    connect(m_auth, &Auth::requestChanged, this, &Display::slotRequestChanged);
    connect(m_auth, &Auth::info, this, &Display::slotAuthInfo);
    connect(m_auth, &Auth::error, this, &Display::slotAuthError);

    // connect login signal
    connect(m_socketServer, &SocketServer::login, this, &Display::login);

    // connect login result signals
    connect(this, &Display::loginFailed, m_socketServer, &SocketServer::loginFailed);
    connect(this, &Display::loginSucceeded, m_socketServer, &SocketServer::loginSucceeded);

    connect(m_greeter, &Greeter::failed, this, &Display::stop);
    connect(m_greeter, &Greeter::ttyFailed, this, [this] {
        ++s_ttyFailures;
        if (s_ttyFailures > 5) {
            QCoreApplication::exit(23);
        }
        // It might be the case that we are trying a tty that has been taken over by a
        // different process. In such a case, switch back to the initial one and try again.
        VirtualTerminal::jumpToVt(PLASMALOGIN_INITIAL_VT, true);
        stop();
    });
    connect(m_greeter, &Greeter::displayServerFailed, this, &Display::displayServerFailed);

    // Load autologin configuration (whether to autologin, user, session, session type)
    if ((mainConfig.Autologin.Relogin.get() || daemonApp->tryLockFirstLogin()) && !mainConfig.Autologin.User.get().isEmpty()) {
        // determine session type
        QString autologinSession = mainConfig.Autologin.Session.get();
        m_autologinSession = Session::create(Session::WaylandSession, autologinSession);
        if (!m_autologinSession.isValid()) {
            m_autologinSession = Session::create(Session::X11Session, autologinSession);
        }
        if (!m_autologinSession.isValid()) {
            qCritical() << "Unable to find autologin session entry" << autologinSession;
        }
    }
}

Display::~Display()
{
    stop();
}

int Display::terminalId() const
{
    return m_auth->isActive() ? m_sessionTerminalId : m_terminalId;
}

QString Display::sessionType() const
{
    return "wayland";
}

Seat *Display::seat() const
{
    return m_seat;
}

bool Display::start()
{
    if (m_started) {
        return true;
    }

    m_started = true;

    // Handle autologin early, unless it needs the display server to be up
    // (rootful X + X11 autologin session).
    // Dave ^ wtf is this, kill this first

    if (m_autologinSession.isValid()) {
        // m_auth->setAutologin(true);
        // don't auth, run a session!
        // DAVE - don't start auth, start session directly
    }

    // no reason for this to be queued, other than porting
    QMetaObject::invokeMethod(this, &Display::displayServerStarted, Qt::QueuedConnection);
    return true;
}

void Display::startSocketServerAndGreeter()
{
    // start socket server
    m_socketServer->start(QString());
    // change the owner and group of the socket to avoid permission denied errors
    struct passwd *pw = getpwnam("plasmalogin");
    if (pw) {
        if (chown(qPrintable(m_socketServer->socketAddress()), pw->pw_uid, pw->pw_gid) == -1) {
            qWarning() << "Failed to change owner of the socket";
            return;
        }
    }

    m_greeter->setSocket(m_socketServer->socketAddress());

    // start greeter
    m_greeter->start();
}

bool Display::handleAutologinFailure()
{
    qWarning() << "Autologin failed!";
    // m_auth->setAutologin(false);
    // For late autologin handling only the greeter needs to be started.

    QMetaObject::invokeMethod(this, &Display::displayServerStarted, Qt::QueuedConnection);
    return true;
}

void Display::displayServerStarted()
{
    // log message
    qDebug() << "Display server started.";

    startSocketServerAndGreeter();
}

void Display::stop()
{
    // check flag
    if (!m_started) {
        return;
    }

    // stop the greeter
    m_greeter->stop();

    m_auth->stop();

    // stop socket server
    m_socketServer->stop();

    // reset flag
    m_started = false;

    // emit signal
    emit stopped();
}

void Display::login(QLocalSocket *socket, const QString &user, const QString &password, const Session &session)
{
    m_socket = socket;

    // the PLASMALOGIN user has special privileges that skip password checking so that we can load the greeter
    // block ever trying to log in as the PLASMALOGIN user
    if (user == QLatin1String("plasmalogin")) {
        emit loginFailed(m_socket);
        return;
    }

    // authenticate
    startAuth(user, password, session);
}

bool Display::startAuth(const QString &user, const QString &password, const Session &session)
{
    qDebug() << "start auth" << "user" << session.isValid() << session.exec();

    if (m_auth->isActive()) {
        qWarning() << "Existing authentication ongoing, aborting";
        return false;
    }

    m_passPhrase = password;

    // sanity check
    if (!session.isValid()) {
        qCritical() << "Invalid session" << session.fileName();
        return false;
    }

    if (session.xdgSessionType().isEmpty()) {
        qCritical() << "Failed to find XDG session type for session" << session.fileName();
        return false;
    }
    if (session.exec().isEmpty()) {
        qCritical() << "Failed to find command for session" << session.fileName();
        return false;
    }

    m_reuseSessionId = QString();

    if (Logind::isAvailable()) {
        OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
        auto reply = manager.ListSessions();
        reply.waitForFinished();

        const auto info = reply.value();
        for (const SessionInfo &s : reply.value()) {
            if (s.userName == user) {
                OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), s.sessionPath.path(), QDBusConnection::systemBus());
                if (session.service() == QLatin1String("plasmalogin") && session.state() == QLatin1String("online")) {
                    m_reuseSessionId = s.sessionId;
                    break;
                }
            }
        }
    }

    // save session desktop file name, we'll use it to set the
    // last session later, in slotAuthenticationFinished()
    m_sessionName = session.fileName();

    m_sessionTerminalId = m_terminalId;

    if (m_greeter->isRunning()) {
        // Create a new VT when we need to have another compositor running
        if (seat()->canTTY()) {
            m_sessionTerminalId = VirtualTerminal::setUpNewVt();
        }
    }

    // some information
    qDebug() << "Session" << m_sessionName << "selected, command:" << session.exec() << "for VT" << m_sessionTerminalId << session.xdgSessionType();

    QProcessEnvironment env;
    env.insert(QStringLiteral("PATH"), mainConfig.Users.DefaultPath.get());
    env.insert(QStringLiteral("XDG_SEAT_PATH"), daemonApp->displayManager()->seatPath(seat()->name()));
    env.insert(QStringLiteral("XDG_SESSION_PATH"), daemonApp->displayManager()->sessionPath(QStringLiteral("Session%1").arg(daemonApp->newSessionId())));
    env.insert(QStringLiteral("DESKTOP_SESSION"), session.desktopSession());
    if (!session.desktopNames().isEmpty()) {
        env.insert(QStringLiteral("XDG_CURRENT_DESKTOP"), session.desktopNames());
    }
    env.insert(QStringLiteral("XDG_SESSION_CLASS"), QStringLiteral("user"));
    env.insert(QStringLiteral("XDG_SESSION_TYPE"), session.xdgSessionType());
    env.insert(QStringLiteral("XDG_SEAT"), seat()->name());
    if (m_sessionTerminalId > 0) {
        env.insert(QStringLiteral("XDG_VTNR"), QString::number(m_sessionTerminalId));
    }
    env.insert(QStringLiteral("XDG_SESSION_DESKTOP"), session.desktopNames());

    m_auth->setUser(user);
    if (m_reuseSessionId.isNull()) {
        // m_auth->setSession(session.exec());
    }
    m_auth->start();
    QString exec = session.exec();

    // Dave - this is silly and probably unsafe, we need one "auth" per login attempt

    connect(m_auth, &Auth::authentication, this, [this, env, exec](QString user, bool success) {
        qDebug() << "auth done" << user << success;
        if (success) {

            // if (!m_reuseSessionId.isNull()) {
            //     -            OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
            //     -            manager.UnlockSession(m_reuseSessionId);
            //     -            manager.ActivateSession(m_reuseSessionId);
            //     -        }


            qDebug() << "STARTING";
            SessionRunner session;
            session.setUser(user);
            session.setExecutable(exec);
            session.insertEnvironment(env);
            session.start();

            return;
        } else if (m_socket) {
            emit loginFailed(m_socket);
        }
    });
    return true;
}


void Display::slotAuthInfo(const QString &message, Auth::Info info)
{
    qWarning() << "Authentication information:" << info << message;

    if (!m_socket) {
        return;
    }

    m_socketServer->informationMessage(m_socket, message);
}

void Display::slotAuthError(const QString &message, Auth::Error error)
{
    qWarning() << "Authentication error:" << error << message;

    if (!m_socket) {
        return;
    }

    m_socketServer->informationMessage(m_socket, message);
    if (error == Auth::ERROR_AUTHENTICATION) {
        emit loginFailed(m_socket);
    }
}

void Display::slotHelperFinished(Auth::HelperExitStatus status)
{
    // Don't restart greeter and display server unless plasmalogin-helper exited
    // with an internal error or the user session finished successfully,
    // we want to avoid greeter from restarting when an authentication
    // error happens (in this case we want to show the message from the
    // greeter
    if (status != Auth::HELPER_AUTH_ERROR) {
        stop();
    }
}

void Display::slotRequestChanged()
{
    if (m_auth->request()->prompts().length() == 1) {
        m_auth->request()->prompts()[0]->setResponse(qPrintable(m_passPhrase));
        m_auth->request()->done();
    } else if (m_auth->request()->prompts().length() == 2) {
        m_auth->request()->prompts()[0]->setResponse(qPrintable(m_auth->user()));
        m_auth->request()->prompts()[1]->setResponse(qPrintable(m_passPhrase));
        m_auth->request()->done();
    }
}

void Display::slotSessionStarted(bool success)
{
    qDebug() << "Session started" << success;
    if (success) {
        QTimer::singleShot(5000, m_greeter, &Greeter::stop);
    }
}
}

#include "moc_Display.cpp"
