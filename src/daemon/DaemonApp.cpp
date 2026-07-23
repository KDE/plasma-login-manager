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

#include "DaemonApp.h"

#include "DisplayManager.h"
#include "SeatManager.h"
#include "UserDatabase.h"
#include <KSignalHandler>

#include "MessageHandler.h"

#include <QDBusConnectionInterface>
#include <QDebug>
#include <QTimer>

#include <iostream>
#include <signal.h>

namespace PLASMALOGIN
{
DaemonApp *DaemonApp::self = nullptr;

DaemonApp::DaemonApp(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    // point instance to this
    self = this;

    qInstallMessageHandler(PLASMALOGIN::DaemonMessageHandler);

    // log message
    qDebug() << "Initializing...";

    // create display manager
    m_displayManager = new DisplayManager(this);

    // create seat manager
    m_seatManager = new SeatManager(this);
    new UserDatabase(this);

    // connect with display manager
    connect(m_seatManager, &SeatManager::seatCreated, m_displayManager, &DisplayManager::AddSeat);
    connect(m_seatManager, &SeatManager::seatRemoved, m_displayManager, &DisplayManager::RemoveSeat);

    // watch SIGINT and SIGTERM and quit on receipt
    auto sig = KSignalHandler::self();
    sig->watchSignal(SIGINT);
    sig->watchSignal(SIGTERM);
    connect(sig, &KSignalHandler::signalReceived, this, [this](int s) {
        if (s == SIGINT || s == SIGTERM) {
            this->quit();
        }
    });
    // log message
    qDebug() << "Starting...";

    // initialize seats only after signals are connected
    m_seatManager->initialize();
}

DisplayManager *DaemonApp::displayManager() const
{
    return m_displayManager;
}

SeatManager *DaemonApp::seatManager() const
{
    return m_seatManager;
}

int DaemonApp::newSessionId()
{
    return m_lastSessionId++;
}

bool DaemonApp::isFirstBoot()
{
    // Whether this boot is a first boot (no soft reboot since power-on) is a
    // machine-global fact that cannot change while the daemon runs, so resolve
    // it once and cache the answer — including the "treat as first boot" error
    // paths, which must stay sticky too.
    if (m_isFirstBoot.has_value()) {
        return m_isFirstBoot.value();
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                      QStringLiteral("/org/freedesktop/systemd1"),
                                                      QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("Get"));

    msg << QStringLiteral("org.freedesktop.systemd1.Manager") << QStringLiteral("SoftRebootsCount");

    const QDBusMessage reply = QDBusConnection::systemBus().call(msg);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "DBus error:" << reply.errorName() << "-" << reply.errorMessage();
        m_isFirstBoot = true;
        return m_isFirstBoot.value();
    }

    const QVariant soft_reboot_count = qvariant_cast<QDBusVariant>(reply.arguments().at(0)).variant();
    if (!soft_reboot_count.isValid()) {
        qWarning() << "DBus variant is invalid:" << reply;
        m_isFirstBoot = true;
        return m_isFirstBoot.value();
    }

    m_isFirstBoot = soft_reboot_count.toUInt() == 0;
    return m_isFirstBoot.value();
};
}

int main(int argc, char **argv)
{
    QStringList arguments;

    for (int i = 0; i < argc; i++) {
        arguments << QString::fromLocal8Bit(argv[i]);
    }

    // create application
    PLASMALOGIN::DaemonApp app(argc, argv);

    // run application
    return app.exec();
}

#include "moc_DaemonApp.cpp"
