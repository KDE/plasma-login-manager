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

#include "SeatManager.h"

#include "DaemonApp.h"
#include "Seat.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusPendingReply>

#include "LogindDBusTypes.h"
#include <Login1Manager.h>
#include <Login1Session.h>

namespace PLASMALOGIN
{

class LogindSeat : public QObject
{
    Q_OBJECT
public:
    LogindSeat(const QString &name, const QDBusObjectPath &objectPath);
    QString name() const;
    bool canGraphical() const;
Q_SIGNALS:
    void canGraphicalChanged(bool);
private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    QString m_name;
    bool m_canGraphical;
};

LogindSeat::LogindSeat(const QString &name, const QDBusObjectPath &objectPath)
    : m_name(name)
    , m_canGraphical(false)
{
    QDBusConnection::systemBus().connect(Logind::serviceName(),
                                         objectPath.path(),
                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                         QStringLiteral("PropertiesChanged"),
                                         this,
                                         SLOT(propertiesChanged(QString, QVariantMap, QStringList)));

    auto canGraphicalMsg =
        QDBusMessage::createMethodCall(Logind::serviceName(), objectPath.path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    canGraphicalMsg << Logind::seatIfaceName() << QStringLiteral("CanGraphical");

    QDBusPendingReply<QVariant> reply = QDBusConnection::systemBus().asyncCall(canGraphicalMsg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply, watcher]() {
        watcher->deleteLater();
        if (!reply.isValid())
            return;

        bool value = reply.value().toBool();
        if (value != m_canGraphical) {
            m_canGraphical = value;
            emit canGraphicalChanged(m_canGraphical);
        }
    });
}

bool LogindSeat::canGraphical() const
{
    return m_canGraphical;
}

QString LogindSeat::name() const
{
    return m_name;
}

void LogindSeat::propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);
    if (interface != Logind::seatIfaceName()) {
        return;
    }

    if (changedProperties.contains(QStringLiteral("CanGraphical"))) {
        m_canGraphical = changedProperties[QStringLiteral("CanGraphical")].toBool();
        emit canGraphicalChanged(m_canGraphical);
    }
}

void SeatManager::initialize()
{
    if (!Logind::isAvailable()) {
        // if we don't have logind/CK2, just create a single seat immediately and don't do any other connections
        createSeat(QStringLiteral("seat0"));
        return;
    }

    // fetch seats
    auto listSeatsMsg = QDBusMessage::createMethodCall(Logind::serviceName(), Logind::managerPath(), Logind::managerIfaceName(), QStringLiteral("ListSeats"));
    QDBusPendingReply<NamedSeatPathList> reply = QDBusConnection::systemBus().asyncCall(listSeatsMsg);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, reply]() {
        watcher->deleteLater();
        const auto seats = reply.value();
        for (const NamedSeatPath &seat : seats) {
            logindSeatAdded(seat.name, seat.path);
        }
    });

    QDBusConnection::systemBus().connect(Logind::serviceName(),
                                         Logind::managerPath(),
                                         Logind::managerIfaceName(),
                                         QStringLiteral("SecureAttentionKey"),
                                         this,
                                         SLOT(logindSecureAttentionKey(QString, QDBusObjectPath)));
    QDBusConnection::systemBus().connect(Logind::serviceName(),
                                         Logind::managerPath(),
                                         Logind::managerIfaceName(),
                                         QStringLiteral("SeatNew"),
                                         this,
                                         SLOT(logindSeatAdded(QString, QDBusObjectPath)));
    QDBusConnection::systemBus().connect(Logind::serviceName(),
                                         Logind::managerPath(),
                                         Logind::managerIfaceName(),
                                         QStringLiteral("SeatRemoved"),
                                         this,
                                         SLOT(logindSeatRemoved(QString, QDBusObjectPath)));
}

void SeatManager::createSeat(const QString &name)
{
    // create a seat
    Seat *seat = new Seat(name, this);

    // add to the list
    m_seats.insert(name, seat);

    // emit signal
    emit seatCreated(name);
}

void SeatManager::removeSeat(const QString &name)
{
    // check if seat exists
    if (!m_seats.contains(name))
        return;

    // remove from the list
    Seat *seat = m_seats.take(name);

    // delete seat
    seat->deleteLater();

    // emit signal
    emit seatRemoved(name);
}

void SeatManager::switchToGreeter(const QString &name)
{
    // check if seat exists
    if (!m_seats.contains(name))
        return;

    // Switch to existing greeter session if available
    if (Logind::isAvailable()) {
        OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
        auto reply = manager.ListSessions();
        reply.waitForFinished();

        const auto info = reply.value();
        for (const SessionInfo &s : reply.value()) {
            if (s.userName == QLatin1String("plasmalogin")) {
                OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), s.sessionPath.path(), QDBusConnection::systemBus());
                if (session.service() == QLatin1String("plasmalogin-greeter") && session.seat().name == name) {
                    session.Activate();
                    return;
                }
            }
        }
    }

    // switch to greeter
    m_seats.value(name)->createDisplay();
}

void PLASMALOGIN::SeatManager::logindSecureAttentionKey(const QString &name, const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath);
    daemonApp->seatManager()->switchToGreeter(name);
}

void PLASMALOGIN::SeatManager::logindSeatAdded(const QString &name, const QDBusObjectPath &objectPath)
{
    auto logindSeat = new LogindSeat(name, objectPath);
    connect(logindSeat, &LogindSeat::canGraphicalChanged, this, [this, logindSeat]() {
        if (logindSeat->canGraphical()) {
            createSeat(logindSeat->name());
        } else {
            removeSeat(logindSeat->name());
        }
    });

    m_systemSeats.insert(name, logindSeat);
}

void PLASMALOGIN::SeatManager::logindSeatRemoved(const QString &name, const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath);
    auto logindSeat = m_systemSeats.take(name);
    delete logindSeat;
    removeSeat(name);
}
}

#include "SeatManager.moc"
