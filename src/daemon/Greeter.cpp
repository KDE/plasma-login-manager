/***************************************************************************
 * SPDX-FileCopyrightText: 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
 * SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>
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

#include "Constants.h"
#include "DaemonApp.h"
#include "Display.h"
#include "DisplayManager.h"
#include "MainConfigLoader.h"
#include "Seat.h"

#include <QStandardPaths>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <VirtualTerminal.h>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

namespace PLASMALOGIN
{
Greeter::Greeter(Display *parent)
    : QObject(parent)
    , m_display(parent)
{
}

Greeter::~Greeter()
{
    stop();
}

void Greeter::setSocket(const QString &socket)
{
    m_socket = socket;
}

bool Greeter::start()
{
    if (m_process) {
        return false;
    }

    QString greeterCommand = QStandardPaths::findExecutable(QStringLiteral("startplasma-login-wayland"));
    // allow overriding for test setups.
    greeterCommand = qEnvironmentVariable("PLASMALOGIN_GREETER_EXEC", greeterCommand);

    if (greeterCommand.isEmpty()) {
        qCritical("Could not find greeter");
    }

    Q_ASSERT(m_display);

    QProcessEnvironment env;
    QProcessEnvironment sysenv = QProcessEnvironment::systemEnvironment();

    insertEnvironmentList({QStringLiteral("LANG"),
                           QStringLiteral("LANGUAGE"),
                           QStringLiteral("LC_CTYPE"),
                           QStringLiteral("LC_NUMERIC"),
                           QStringLiteral("LC_TIME"),
                           QStringLiteral("LC_COLLATE"),
                           QStringLiteral("LC_MONETARY"),
                           QStringLiteral("LC_MESSAGES"),
                           QStringLiteral("LC_PAPER"),
                           QStringLiteral("LC_NAME"),
                           QStringLiteral("LC_ADDRESS"),
                           QStringLiteral("LC_TELEPHONE"),
                           QStringLiteral("LC_MEASUREMENT"),
                           QStringLiteral("LC_IDENTIFICATION"),
                           QStringLiteral("LD_LIBRARY_PATH"),
                           QStringLiteral("QML2_IMPORT_PATH"),
                           QStringLiteral("QT_PLUGIN_PATH"),
                           QStringLiteral("XDG_DATA_DIRS")},
                          sysenv,
                          env);

    env.insert(QStringLiteral("PATH"), PlasmaLogin::config()->defaultPath());
    env.insert(QStringLiteral("XDG_SEAT"), m_display->seat()->name());
    env.insert(QStringLiteral("XDG_SEAT_PATH"), daemonApp->displayManager()->seatPath(m_display->seat()->name()));
    env.insert(QStringLiteral("XDG_SESSION_PATH"), daemonApp->displayManager()->sessionPath(QStringLiteral("Session%1").arg(daemonApp->newSessionId())));
    if (m_display->seat()->name() == QLatin1String("seat0") && m_display->terminalId() > 0) {
        env.insert(QStringLiteral("XDG_VTNR"), QString::number(m_display->terminalId()));
    }
    env.insert(QStringLiteral("XDG_SESSION_CLASS"), QStringLiteral("greeter"));
    env.insert(QStringLiteral("XDG_SESSION_TYPE"), m_display->sessionType());
    env.insert(QStringLiteral("SDDM_SOCKET"), m_socket);
    env.insert(u"QT_NO_XDG_DESKTOP_PORTAL"_s, u"1"_s);

    qDebug() << "Greeter starting...";

    m_unitName = u"plasmalogin-greeter@tty%1.service"_s.arg(m_display->terminalId());

    m_process = new QProcess(this);
    connect(m_process, &QProcess::started, this, [] {
        qDebug() << "Greeter session started successfully";
    });
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        switch (exitStatus) {
        case QProcess::NormalExit:
            onHelperFinished(static_cast<Auth::HelperExitStatus>(exitCode));
            break;
        case QProcess::CrashExit:
            qWarning() << "Greeter crashed with exit code" << exitCode;
            onHelperFinished(Auth::HELPER_OTHER_ERROR);
            break;
        }
    });
    m_process->setProgram(u"systemd-run"_s);
    m_process->setArguments([&] {
        auto arguments = QStringList{
            u"--unit="_s + m_unitName,
            u"--description=Plasma Login Manager Greeter (TTY %1)"_s.arg(m_display->terminalId()),
            u"--service-type=simple"_s,
            u"--slice-inherit"_s,
            u"--collect"_s, // Remove the unit after we are done

            // Unit Configuration
            u"--property=StopPropagatedFrom=plasmalogin.service"_s,
            u"--property=Restart=no"_s, // Process gets managed by the Greeter class, not systemd
            u"--property=User=plasmalogin"_s,
            u"--property=PAMName=plasmalogin-greeter"_s,

            // TTY Configuration
            u"--property=TTYPath=/dev/tty%1"_s.arg(m_display->terminalId()),
            u"--property=TTYReset=yes"_s, // Reset tty before and after
            u"--property=TTYVHangup=yes"_s, // Hangup prior clients
            u"--property=TTYVTDisallocate=yes"_s, // Clear VT scrollback etc.

            // Utmp tracking. Nobody knows what these do exactly!
            u"--property=UtmpIdentifier=tty%1"_s.arg(m_display->terminalId()),
            u"--property=UtmpMode=user"_s,

            // Output
            u"--property=StandardInput=tty-fail"_s,
            u"--property=StandardOutput=journal"_s,
            u"--property=StandardError=journal"_s,

            u"--json=short"_s,
        };
        for (const auto &var : env.toStringList()) {
            arguments.append(u"--setenv=%1"_s.arg(var));
        }
        arguments.append(greeterCommand);
        return arguments;
    }());
    m_process->start();

    return true;
}

void Greeter::insertEnvironmentList(QStringList names, QProcessEnvironment sourceEnv, QProcessEnvironment &targetEnv)
{
    for (QStringList::const_iterator it = names.constBegin(); it != names.constEnd(); ++it) {
        if (sourceEnv.contains(*it)) {
            targetEnv.insert(*it, sourceEnv.value(*it));
        }
    }
}

void Greeter::stop()
{
    if (!m_process) {
        return;
    }

    qDebug() << "Greeter stopping...";

    // We no longer care about its outcome. Let's ignore all signals to avoid confusion.
    // This also avoids problems with waitForFinished doing event looping and maybe deleting things out from under us.
    m_process->disconnect(this);

    QProcess term;
    term.setProgram(u"systemctl"_s);
    term.setArguments({u"kill"_s, u"--kill-whom=all"_s, u"--signal=SIGTERM"_s, m_unitName});
    term.start();
    if (!term.waitForFinished((250ms).count())) {
        qWarning() << "Greeter did not stop in time, sending SIGKILL";
        QProcess kill;
        kill.setProgram(u"systemctl"_s);
        kill.setArguments({u"kill"_s, u"--kill-whom=all"_s, u"--signal=SIGKILL"_s, m_unitName});
        kill.start();
        if (!kill.waitForFinished((100ms).count())) {
            qWarning() << "Greeter did not stop in time, killing it";
        }
    }

    qDebug() << "Greeter stopped.";

    m_process->deleteLater();
    m_process = nullptr;
}

void Greeter::onHelperFinished(Auth::HelperExitStatus status)
{
    qDebug() << "Greeter stopped." << status;

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    if (status == Auth::HELPER_TTY_ERROR) {
        Q_EMIT ttyFailed();
    } else if (status == Auth::HELPER_SESSION_ERROR) {
        Q_EMIT failed();
    }
}

bool Greeter::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}
}

#include "moc_Greeter.cpp"
