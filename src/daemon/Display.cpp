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

#include "DaemonApp.h"
#include "DisplayManager.h"
#include "Greeter.h"
#include "MainConfigLoader.h"
#include "Seat.h"
#include "SocketServer.h"

#include <QDebug>
#include <QFile>
#include <QLocalSocket>
#include <QTimer>

#include <pwd.h>
#include <sys/time.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include "VirtualTerminal.h"
#include "config.h"

static int s_ttyFailures = 0;

namespace PLASMALOGIN
{
Display::Display(Seat *parent)
    : QObject(parent)
    , m_auth(new Auth(this))
    , m_seat(parent)
{
    if (seat()->canTTY()) {
        m_terminalId = seat()->availableVt();
    }
    qDebug("Using VT %d", m_terminalId);

    // respond to authentication requests
    m_auth->setVerbose(true);
    connect(m_auth, &Auth::requestChanged, this, &Display::slotRequestChanged);
    connect(m_auth, &Auth::authentication, this, &Display::slotAuthenticationFinished);
    connect(m_auth, &Auth::sessionStarted, this, &Display::slotSessionStarted);
    connect(m_auth, &Auth::finished, this, &Display::slotHelperFinished);
}

Display::~Display()
{
    disconnect(m_auth, &Auth::finished, this, &Display::slotHelperFinished);
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

bool Display::hasGreeter() const
{
    return false;
}

bool Display::start()
{
    if (m_started) {
        return true;
    }

    m_started = true;
    return true;
}

void Display::stop()
{
    if (!m_started) {
        return;
    }

    m_auth->stop();
    m_started = false;
    emit stopped();
}

GreeterDisplay::GreeterDisplay(Seat *parent)
    : Display(parent)
    , m_socketServer(new SocketServer(this))
    , m_greeter(new Greeter(this))
{
    connect(m_auth, &Auth::info, this, &GreeterDisplay::slotAuthInfo);
    connect(m_auth, &Auth::error, this, &GreeterDisplay::slotAuthError);

    connect(m_socketServer, &SocketServer::login, this, &GreeterDisplay::login);
    connect(this, &Display::loginFailed, m_socketServer, &SocketServer::loginFailed);
    connect(this, &Display::loginSucceeded, m_socketServer, &SocketServer::loginSucceeded);

    connect(m_greeter, &Greeter::failed, this, &GreeterDisplay::stop);
    connect(m_greeter, &Greeter::ttyFailed, this, [this] {
        ++s_ttyFailures;
        if (s_ttyFailures > 5) {
            QCoreApplication::exit(23);
        }
        VirtualTerminal::jumpToVt(PLASMALOGIN_INITIAL_VT, true);
        stop();
    });
}

bool GreeterDisplay::hasGreeter() const
{
    return m_greeter->isRunning();
}

bool GreeterDisplay::start()
{
    if (m_started) {
        return true;
    }

    if (!Display::start()) {
        return false;
    }

    QMetaObject::invokeMethod(this, &GreeterDisplay::displayServerStarted, Qt::QueuedConnection);
    return true;
}

void GreeterDisplay::startSocketServerAndGreeter()
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

void GreeterDisplay::displayServerStarted()
{
    qDebug() << "Display server started.";
    startSocketServerAndGreeter();
}

void GreeterDisplay::stop()
{
    if (!m_started) {
        return;
    }

    m_greeter->stop();
    m_socketServer->stop();
    Display::stop();
}

void GreeterDisplay::login(QLocalSocket *socket, const QString &user, const QString &password, const Session &session)
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

    m_reuseSessionId = seat()->reusableSessionId(user);

    // save session desktop file name, we'll use it to set the
    // last session later, in slotAuthenticationFinished()
    m_sessionName = session.fileName();

    m_sessionTerminalId = m_terminalId;

    if (hasGreeter()) {
        // Create a new VT when we need to have another compositor running
        if (seat()->canTTY()) {
            m_sessionTerminalId = VirtualTerminal::setUpNewVt();
        }
    }

    // some information
    qDebug() << "Session" << m_sessionName << "selected, command:" << session.exec() << "for VT" << m_sessionTerminalId << session.xdgSessionType();

    QProcessEnvironment env;
    env.insert(QStringLiteral("PATH"), PlasmaLogin::config()->defaultPath());
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
        m_auth->setSession(session.exec());
    }
    m_auth->insertEnvironment(env);
    m_auth->start();

    return true;
}

void Display::slotAuthenticationFinished(const QString &user, bool success)
{
    if (m_auth->autologin() && !success) {
        handleAutologinFailure();
        return;
    }

    if (success) {
        qDebug() << "Authentication for user " << user << " successful";

        if (!m_reuseSessionId.isNull()) {
            seat()->activateSession(m_reuseSessionId);
        }

        if (m_socket) {
            emit loginSucceeded(m_socket);
        }
    } else if (m_socket) {
        qDebug() << "Authentication for user " << user << " failed";
        emit loginFailed(m_socket);
    }
    m_socket = nullptr;
}

void GreeterDisplay::slotAuthInfo(const QString &message, Auth::Info info)
{
    qWarning() << "Authentication information:" << info << message;

    if (!m_socket) {
        return;
    }

    m_socketServer->informationMessage(m_socket, message);
}

void GreeterDisplay::slotAuthError(const QString &message, Auth::Error error)
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

void Display::handleAutologinFailure()
{
    qWarning() << "Autologin failed!";
    m_auth->setAutologin(false);
    stop();
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

void Display::slotSessionStarted(bool)
{
}

void GreeterDisplay::slotSessionStarted(bool success)
{
    qDebug() << "Session started" << success;
    if (success) {
        QTimer::singleShot(5000, m_greeter, &Greeter::stop);
    }
}

AutoLoginDisplay::AutoLoginDisplay(Seat *parent)
    : Display(parent)
{
}

void AutoLoginDisplay::setAutoLogin(const QString &user, const QString &session)
{
    m_autologinUser = user;
    m_autologinSession = Session();
    if (user.isEmpty()) {
        return;
    }

    m_autologinSession = Session::create(Session::WaylandSession, session);
    if (!m_autologinSession.isValid()) {
        m_autologinSession = Session::create(Session::X11Session, session);
    }
    if (!m_autologinSession.isValid()) {
        qCritical() << "Unable to find autologin session entry" << session << "for user" << user << "on seat" << seat()->name()
                    << "— falling back to the greeter";
    }
}

bool AutoLoginDisplay::start()
{
    if (m_started) {
        return true;
    }

    if (!Display::start()) {
        return false;
    }

    if (!m_autologinSession.isValid()) {
        return false;
    }

    m_auth->setAutologin(true);
    if (startAuth(m_autologinUser, QString(), m_autologinSession)) {
        return true;
    }

    handleAutologinFailure();
    return false;
}

void AutoLoginDisplay::handleAutologinFailure()
{
    qWarning() << "Autologin failed!";
    m_auth->setAutologin(false);
    stop();
}
}

#include "moc_Display.cpp"
