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
    QString activeTty() const;
    uint activeVt() const;
Q_SIGNALS:
    void canGraphicalChanged(bool);
private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void updateActiveSession(const NamedSessionPath &activeSession);

    QString m_name;
    bool m_canGraphical;
    QString m_activeTty;
    uint m_activeVt = 0;
};

class LogindSession : public QObject
{
    Q_OBJECT
public:
    LogindSession(const QString &id, const QDBusObjectPath &objectPath, const QString &seatName = QString());
    QString id() const;
    QString seatName() const;
    QString tty() const;
    uint vt() const;

private:
    QString m_id;
    QString m_seatName;
    QString m_tty;
    uint m_vt = 0;
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
        if (!reply.isValid()) {
            return;
        }

        bool value = reply.value().toBool();
        if (value != m_canGraphical) {
            m_canGraphical = value;
            emit canGraphicalChanged(m_canGraphical);
        }
    });

    auto activeSessionMsg =
        QDBusMessage::createMethodCall(Logind::serviceName(), objectPath.path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    activeSessionMsg << Logind::seatIfaceName() << QStringLiteral("ActiveSession");

    QDBusPendingReply<QVariant> activeSessionReply = QDBusConnection::systemBus().asyncCall(activeSessionMsg);
    QDBusPendingCallWatcher *activeSessionWatcher = new QDBusPendingCallWatcher(activeSessionReply);
    connect(activeSessionWatcher, &QDBusPendingCallWatcher::finished, this, [this, activeSessionReply, activeSessionWatcher]() {
        activeSessionWatcher->deleteLater();
        if (!activeSessionReply.isValid()) {
            return;
        }

        updateActiveSession(qdbus_cast<NamedSessionPath>(activeSessionReply.value()));
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

QString LogindSeat::activeTty() const
{
    return m_activeTty;
}

uint LogindSeat::activeVt() const
{
    return m_activeVt;
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
    if (changedProperties.contains(QStringLiteral("ActiveSession"))) {
        updateActiveSession(qdbus_cast<NamedSessionPath>(changedProperties[QStringLiteral("ActiveSession")]));
    }
}

void LogindSeat::updateActiveSession(const NamedSessionPath &activeSession)
{
    const QString activeSessionPath = activeSession.path.path();
    if (activeSessionPath.isEmpty() || activeSessionPath == QLatin1String("/")) {
        return;
    }

    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), activeSessionPath, QDBusConnection::systemBus());
    m_activeTty = session.tTY();
    m_activeVt = session.vTNr();
}

LogindSession::LogindSession(const QString &id, const QDBusObjectPath &objectPath, const QString &seatName)
    : m_id(id)
    , m_seatName(seatName)
{
    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), objectPath.path(), QDBusConnection::systemBus());
    if (m_seatName.isEmpty()) {
        m_seatName = session.seat().name;
    }
    m_tty = session.tTY();
    m_vt = session.vTNr();
}

QString LogindSession::id() const
{
    return m_id;
}

QString LogindSession::seatName() const
{
    return m_seatName;
}

QString LogindSession::tty() const
{
    return m_tty;
}

uint LogindSession::vt() const
{
    return m_vt;
}

void SeatManager::initialize()
{
    if (!Logind::isAvailable()) {
        // if we don't have logind/CK2, just create a single seat immediately and don't do any other connections
        createSeat(QStringLiteral("seat0"));
        return;
    }

    auto logind = new OrgFreedesktopLogin1ManagerInterface(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus(), this);
    QDBusPendingReply<NamedSeatPathList> reply = logind->ListSeats();
    QDBusPendingReply<SessionInfoList> sessionsReply = logind->ListSessions();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, reply]() {
        watcher->deleteLater();
        const auto seats = reply.value();
        for (const NamedSeatPath &seat : seats) {
            logindSeatAdded(seat.name, seat.path);
        }
    });
    QDBusPendingCallWatcher *sessionsWatcher = new QDBusPendingCallWatcher(sessionsReply);
    connect(sessionsWatcher, &QDBusPendingCallWatcher::finished, this, [this, sessionsWatcher, sessionsReply]() {
        sessionsWatcher->deleteLater();
        const auto sessions = sessionsReply.value();
        for (const SessionInfo &session : sessions) {
            if (session.seatId.isEmpty()) {
                continue;
            }
            auto *logindSession = new LogindSession(session.sessionId, session.sessionPath, session.seatId);
            m_systemSessions.insert(session.sessionId, logindSession);
        }
    });

    connect(logind, &OrgFreedesktopLogin1ManagerInterface::SeatNew, this, &SeatManager::logindSeatAdded);
    connect(logind, &OrgFreedesktopLogin1ManagerInterface::SeatRemoved, this, &SeatManager::logindSeatRemoved);
    connect(logind, &OrgFreedesktopLogin1ManagerInterface::SessionNew, this, &SeatManager::logindSessionAdded);
    connect(logind, &OrgFreedesktopLogin1ManagerInterface::SessionRemoved, this, &SeatManager::logindSessionRemoved);
    connect(logind, &OrgFreedesktopLogin1ManagerInterface::SecureAttentionKey, this, &SeatManager::logindSecureAttentionKey);
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
    if (!m_seats.contains(name)) {
        return;
    }

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
    if (!m_seats.contains(name)) {
        return;
    }

    m_seats.value(name)->switchToGreeter();
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

void PLASMALOGIN::SeatManager::logindSessionAdded(const QString &id, const QDBusObjectPath &objectPath)
{
    auto *logindSession = new LogindSession(id, objectPath);
    if (logindSession->seatName().isEmpty()) {
        delete logindSession;
        return;
    }

    auto *oldSession = m_systemSessions.take(id);
    delete oldSession;
    m_systemSessions.insert(id, logindSession);
}

void PLASMALOGIN::SeatManager::logindSessionRemoved(const QString &id, const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath);

    auto *logindSession = m_systemSessions.take(id);
    if (!logindSession) {
        return;
    }

    const QString seatName = logindSession->seatName();
    auto *logindSeat = m_systemSeats.value(seatName);
    const bool shouldSwitchToGreeter = logindSeat && !logindSeat->activeTty().isEmpty() && logindSeat->activeTty() == logindSession->tty();
    delete logindSession;

    if (!shouldSwitchToGreeter || !m_seats.contains(seatName)) {
        return;
    }

    switchToGreeter(seatName);
}
}

#include "SeatManager.moc"

#include "moc_SeatManager.cpp"
