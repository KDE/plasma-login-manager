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

#include "Constants.h"
#include "DaemonApp.h"
#include "Display.h"
#include "DisplayManager.h"
#include "MainConfigLoader.h"
#include "Seat.h"
#include "SessionRunner.h"

#include <QStandardPaths>
#include <QtCore/QDebug>
#include <VirtualTerminal.h>

namespace PLASMALOGIN
{
Greeter::Greeter(Display *parent)
    : QObject(parent)
    , m_display(parent)
    , m_sessionRunner(new SessionRunner(this))
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
    // check flag
    if (m_started) {
        return false;
    }

    QString greeterCommand = QStandardPaths::findExecutable(QStringLiteral("startplasma-login-wayland"));
    // allow overriding for test setups.
    greeterCommand = qEnvironmentVariable("PLASMALOGIN_GREETER_EXEC", greeterCommand);

    if (greeterCommand.isEmpty()) {
        qCritical("Could not find greeter");
    }

    Q_ASSERT(m_display);
    {
        // greeter environment
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
        env.insert(QStringLiteral("QT_NO_XDG_DESKTOP_PORTAL"), 1);
        m_sessionRunner->insertEnvironment(env);

        // log message
        qDebug() << "Greeter starting...";

        // start greeter
        m_sessionRunner->setUser(QStringLiteral("plasmalogin"));
        m_sessionRunner->setGreeter(true);
        m_sessionRunner->setExecutable(greeterCommand);
        m_sessionRunner->setTty(m_display->terminalId());
        m_started = m_sessionRunner->start();
        if (m_started && m_display->seat()->canTTY() && m_display->terminalId() > 0) {
            VirtualTerminal::jumpToVt(m_display->terminalId(), true);
        }
    }

    // return success
    return m_started;
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
    // check flag
    if (!m_started) {
        return;
    }

    // log message
    qDebug() << "Greeter stopping...";
    if (m_sessionRunner && m_sessionRunner->isRunning()) {
        const bool stopped = m_sessionRunner->stop();
        Q_UNUSED(stopped);
        m_started = false;
        qDebug() << "Greeter stopped.";
    }
}

bool Greeter::isRunning() const
{
    return m_started || (m_sessionRunner && m_sessionRunner->isRunning());
}
}

#include "moc_Greeter.cpp"
