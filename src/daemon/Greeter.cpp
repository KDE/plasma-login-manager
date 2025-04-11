/***************************************************************************
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

#include "Greeter.h"

#include "Configuration.h"
#include "Constants.h"
#include "DaemonApp.h"
#include "DisplayManager.h"
#include "Seat.h"
#include "Display.h"
#include "XorgUserDisplayServer.h"
#include "WaylandDisplayServer.h"

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QStandardPaths>
#include <VirtualTerminal.h>

namespace PLASMALOGIN {
    Greeter::Greeter(Display *parent)
        : QObject(parent)
        , m_display(parent)
    {
    }

    Greeter::~Greeter() {
        stop();

    }

    void Greeter::setSocket(const QString &socket) {
        m_socket = socket;
    }

    QString Greeter::displayServerCommand() const
    {
        return m_displayServerCmd;
    }

    void Greeter::setDisplayServerCommand(const QString &cmd)
    {
        m_displayServerCmd = cmd;
    }

    bool Greeter::start() {
        // check flag
        if (m_started)
            return false;


        QString greeterCommand = QStandardPaths::findExecutable(QStringLiteral("startplasma-login-wayland"));
        // allow overriding for test setups.
        greeterCommand = qEnvironmentVariable("PLASMALOGIN_GREETER_EXEC", greeterCommand);

        if (greeterCommand.isEmpty()) {
            qCritical("Could not find greeter");
        }

        Q_ASSERT(m_display);
        auto *displayServer = m_display->displayServer();

        {
            // authentication
            m_auth = new Auth(this);
            m_auth->setVerbose(true);
            connect(m_auth, &Auth::requestChanged, this, &Greeter::onRequestChanged);
            connect(m_auth, &Auth::sessionStarted, this, &Greeter::onSessionStarted);
            connect(m_auth, &Auth::displayServerReady, this, &Greeter::onDisplayServerReady);
            connect(m_auth, &Auth::finished, this, &Greeter::onHelperFinished);
            connect(m_auth, &Auth::info, this, &Greeter::authInfo);
            connect(m_auth, &Auth::error, this, &Greeter::authError);

            // greeter environment
            QProcessEnvironment env;
            QProcessEnvironment sysenv = QProcessEnvironment::systemEnvironment();

            insertEnvironmentList({QStringLiteral("LANG"), QStringLiteral("LANGUAGE"),
                                   QStringLiteral("LC_CTYPE"), QStringLiteral("LC_NUMERIC"), QStringLiteral("LC_TIME"), QStringLiteral("LC_COLLATE"),
                                   QStringLiteral("LC_MONETARY"), QStringLiteral("LC_MESSAGES"), QStringLiteral("LC_PAPER"), QStringLiteral("LC_NAME"),
                                   QStringLiteral("LC_ADDRESS"), QStringLiteral("LC_TELEPHONE"), QStringLiteral("LC_MEASUREMENT"), QStringLiteral("LC_IDENTIFICATION"),
                                   QStringLiteral("LD_LIBRARY_PATH"),
                                   QStringLiteral("QML2_IMPORT_PATH"),
                                   QStringLiteral("QT_PLUGIN_PATH"),
                                   QStringLiteral("XDG_DATA_DIRS")
            }, sysenv, env);

            env.insert(QStringLiteral("PATH"), mainConfig.Users.DefaultPath.get());
            env.insert(QStringLiteral("XDG_SEAT"), m_display->seat()->name());
            env.insert(QStringLiteral("XDG_SEAT_PATH"), daemonApp->displayManager()->seatPath(m_display->seat()->name()));
            env.insert(QStringLiteral("XDG_SESSION_PATH"), daemonApp->displayManager()->sessionPath(QStringLiteral("Session%1").arg(daemonApp->newSessionId())));
            if (m_display->seat()->name() == QLatin1String("seat0") && m_display->terminalId() > 0)
                env.insert(QStringLiteral("XDG_VTNR"), QString::number(m_display->terminalId()));
            env.insert(QStringLiteral("XDG_SESSION_CLASS"), QStringLiteral("greeter"));
            env.insert(QStringLiteral("XDG_SESSION_TYPE"), m_display->sessionType());
            env.insert(QStringLiteral("SDDM_SOCKET"), m_socket);

            m_auth->insertEnvironment(env);

            // log message
            qDebug() << "Greeter starting...";

            // start greeter
            m_auth->setUser(QStringLiteral("plasmalogin"));
            m_auth->setDisplayServerCommand(m_displayServerCmd);
            m_auth->setGreeter(true);
            m_auth->setSession(greeterCommand);
            m_auth->start();
        }

        // return success
        return true;
    }

    void Greeter::insertEnvironmentList(QStringList names, QProcessEnvironment sourceEnv, QProcessEnvironment &targetEnv) {
        for (QStringList::const_iterator it = names.constBegin(); it != names.constEnd(); ++it)
            if (sourceEnv.contains(*it))
                targetEnv.insert(*it, sourceEnv.value(*it));
    }

    void Greeter::stop() {
        // check flag
        if (!m_started)
            return;

        // log message
        qDebug() << "Greeter stopping...";
        m_auth->stop();
    }

    void Greeter::finished() {
        // check flag
        if (!m_started)
            return;

        // reset flag
        m_started = false;

        // log message
        qDebug() << "Greeter stopped.";

        // clean up
        if (m_process) {
            m_process->deleteLater();
            m_process = nullptr;
        }
    }

    void Greeter::onRequestChanged() {
        m_auth->request()->setFinishAutomatically(true);
    }

    void Greeter::onSessionStarted(bool success) {
        // set flag
        m_started = success;

        // log message
        if (success)
            qDebug() << "Greeter session started successfully";
        else
            qDebug() << "Greeter session failed to start";
    }

    void Greeter::onDisplayServerReady(const QString &displayName)
    {
        auto *displayServer = m_display->displayServer();

        auto *wayland = qobject_cast<WaylandDisplayServer *>(displayServer);
        if (wayland)
            wayland->setDisplayName(displayName);
    }

    void Greeter::onHelperFinished(Auth::HelperExitStatus status) {
        // reset flag
        m_started = false;

        // log message
        qDebug() << "Greeter stopped." << status;

        // clean up
        m_auth->deleteLater();
        m_auth = nullptr;

        if (status == Auth::HELPER_DISPLAYSERVER_ERROR) {
            Q_EMIT displayServerFailed();
        } else if (status == Auth::HELPER_TTY_ERROR) {
            Q_EMIT ttyFailed();
        } else if (status == Auth::HELPER_SESSION_ERROR) {
            Q_EMIT failed();
        }
    }

    bool Greeter::isRunning() const {
        return (m_process && m_process->state() == QProcess::Running)
            || (m_auth && m_auth->isActive());
    }

    void Greeter::onReadyReadStandardError()
    {
        if (m_process) {
            qDebug() << "Greeter errors:" << m_process->readAllStandardError().constData();
        }
    }

    void Greeter::onReadyReadStandardOutput()
    {
        if (m_process) {
            qDebug() << "Greeter output:" << m_process->readAllStandardOutput().constData();
        }
    }

    void Greeter::authInfo(const QString &message, Auth::Info info) {
        Q_UNUSED(info);
        qDebug() << "Information from greeter session:" << message;
    }

    void Greeter::authError(const QString &message, Auth::Error error) {
        Q_UNUSED(error);
        qWarning() << "Error from greeter session:" << message;
    }
}
