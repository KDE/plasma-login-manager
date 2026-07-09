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

using namespace Qt::StringLiterals;

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
    [[nodiscard]] bool hasActiveTTY() const
    {
        return !m_activeTty.isEmpty();
    }

Q_SIGNALS:
    void canGraphicalChanged(bool canGraphical);
    void hasActiveTTYChanged();

private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void updateActiveSession(const NamedSessionPath &activeSession);

    QString m_name;
    bool m_canGraphical = false;
    QString m_activeTty;
};

class LogindSession
{
public:
    LogindSession(const QDBusObjectPath &objectPath, const QString &seatName = QString());
    QString seatName() const;
    QDBusObjectPath objectPath() const
    {
        return m_objectPath;
    }
    [[nodiscard]] bool isGreeter() const
    {
        return m_isGreeter;
    }

private:
    QString m_seatName;
    QString m_tty;
    QDBusObjectPath m_objectPath;
    bool m_isGreeter = false;
};

LogindSeat::LogindSeat(const QString &name, const QDBusObjectPath &objectPath)
    : m_name(name)
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

    QDBusPendingReply<QVariant> canGraphicalReply = QDBusConnection::systemBus().asyncCall(canGraphicalMsg);
    auto canGraphicalWatcher = new QDBusPendingCallWatcher(canGraphicalReply);
    connect(canGraphicalWatcher, &QDBusPendingCallWatcher::finished, this, [this, canGraphicalReply, canGraphicalWatcher]() {
        canGraphicalWatcher->deleteLater();
        if (!canGraphicalReply.isValid()) {
            return;
        }

        bool value = canGraphicalReply.value().toBool();
        if (value != m_canGraphical) {
            m_canGraphical = value;
            emit canGraphicalChanged(m_canGraphical);
        }
    });

    auto activeSessionMsg =
        QDBusMessage::createMethodCall(Logind::serviceName(), objectPath.path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    activeSessionMsg << Logind::seatIfaceName() << QStringLiteral("ActiveSession");

    QDBusPendingReply<QVariant> activeSessionReply = QDBusConnection::systemBus().asyncCall(activeSessionMsg);
    auto activeSessionWatcher = new QDBusPendingCallWatcher(activeSessionReply);
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

void LogindSeat::propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    qInfo() << "Properties changed for interface:" << interface << "changed properties:" << changedProperties
            << "invalidated properties:" << invalidatedProperties;

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
    qDebug() << "Updating active session for session path:" << activeSession.path.path() << activeSession.name;

    const QString activeSessionPath = activeSession.path.path();
    if (activeSessionPath.isEmpty() || activeSessionPath == QLatin1String("/")) {
        m_activeTty.clear();
        Q_EMIT hasActiveTTYChanged();
        return;
    }

    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), activeSessionPath, QDBusConnection::systemBus());
    m_activeTty = session.tTY();
    Q_EMIT hasActiveTTYChanged();
}

LogindSession::LogindSession(const QDBusObjectPath &objectPath, const QString &seatName)
    : m_seatName(seatName)
    , m_objectPath(objectPath)
{
    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), objectPath.path(), QDBusConnection::systemBus());
    if (m_seatName.isEmpty()) {
        m_seatName = session.seat().name;
    }

    auto userName = session.name(); // the Name property is indeed the user's name
    auto service = session.service();
    m_isGreeter = (userName == "plasmalogin"_L1 && service == "plasmalogin-greeter"_L1);
}

QString LogindSession::seatName() const
{
    return m_seatName;
}

SeatManager::SeatManager(QObject *parent)
    : QObject(parent)
    , m_logind(new OrgFreedesktopLogin1ManagerInterface(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus(), this))
{
}

void SeatManager::initialize()
{
    if (!Logind::isAvailable()) {
        // if we don't have logind/CK2, just create a single seat immediately and don't do any other connections
        createSeat(QStringLiteral("seat0"));
        return;
    }

    auto seatsReply = m_logind->ListSeats();
    auto seatsWatcher = new QDBusPendingCallWatcher(seatsReply);
    connect(seatsWatcher, &QDBusPendingCallWatcher::finished, this, [this, seatsWatcher, seatsReply]() {
        seatsWatcher->deleteLater();
        const auto seats = seatsReply.value();
        for (const NamedSeatPath &seat : seats) {
            logindSeatAdded(seat.name, seat.path);
        }
    });

    auto sessionsReply = m_logind->ListSessions();
    auto sessionsWatcher = new QDBusPendingCallWatcher(sessionsReply);
    connect(sessionsWatcher, &QDBusPendingCallWatcher::finished, this, [this, sessionsWatcher, sessionsReply]() {
        sessionsWatcher->deleteLater();
        const auto sessions = sessionsReply.value();
        for (const SessionInfo &session : sessions) {
            trackSession(session.sessionId, std::make_shared<LogindSession>(session.sessionPath, session.seatId));
        }
    });

    connect(m_logind, &OrgFreedesktopLogin1ManagerInterface::SeatNew, this, &SeatManager::logindSeatAdded);
    connect(m_logind, &OrgFreedesktopLogin1ManagerInterface::SeatRemoved, this, &SeatManager::logindSeatRemoved);
    connect(m_logind, &OrgFreedesktopLogin1ManagerInterface::SessionNew, this, &SeatManager::logindSessionAdded);
    connect(m_logind, &OrgFreedesktopLogin1ManagerInterface::SessionRemoved, this, &SeatManager::logindSessionRemoved);
    connect(m_logind, &OrgFreedesktopLogin1ManagerInterface::SecureAttentionKey, this, &SeatManager::logindSecureAttentionKey);
}

void SeatManager::createSeat(const QString &name)
{
    // create a seat
    Seat *seat = new Seat(name, this);

    // add to the list
    m_graphicalSeats.insert(name, seat);

    // emit signal
    emit seatCreated(name);
}

void SeatManager::removeSeat(const QString &name)
{
    // check if seat exists
    if (!m_graphicalSeats.contains(name)) {
        return;
    }

    // remove from the list
    Seat *seat = m_graphicalSeats.take(name);

    // delete seat
    seat->deleteLater();

    // emit signal
    emit seatRemoved(name);
}

void SeatManager::switchToGreeter(const QString &name)
{
    if (m_logind->preparingForShutdown()) { // no need during shutdown
        return;
    }

    if (!m_graphicalSeats.contains(name)) {
        qWarning() << "Seat" << name << "does not exist in graphical seats, yet we were asked to switch to it. Seems like a bug.";
        return;
    }

    m_graphicalSeats.value(name)->switchToGreeter();
}

void SeatManager::logindSecureAttentionKey(const QString &name, [[maybe_unused]] const QDBusObjectPath &objectPath)
{
    daemonApp->seatManager()->switchToGreeter(name);
}

void SeatManager::logindSeatAdded(const QString &name, const QDBusObjectPath &objectPath)
{
    auto logindSeat = std::make_shared<LogindSeat>(name, objectPath);
    connect(logindSeat.get(), &LogindSeat::canGraphicalChanged, this, [this, logindSeat = logindSeat.get() /* don't hold a shared_ptr! */]() {
        if (logindSeat->canGraphical()) {
            createSeat(logindSeat->name());
        } else {
            removeSeat(logindSeat->name());
        }
    });
    connect(logindSeat.get(), &LogindSeat::hasActiveTTYChanged, this, [this, logindSeat = logindSeat.get() /* don't hold a shared_ptr! */]() {
        maybeSwitchToGreeter(logindSeat);
    });

    m_systemSeats.insert(name, std::move(logindSeat));
}

void SeatManager::logindSeatRemoved(const QString &name, const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath);
    m_systemSeats.remove(name);
    removeSeat(name);
}

void PLASMALOGIN::SeatManager::logindSessionAdded(const QString &id, const QDBusObjectPath &objectPath)
{
    auto session = std::make_shared<LogindSession>(objectPath);
    trackSession(id, session);
}

void PLASMALOGIN::SeatManager::logindSessionRemoved(const QString &id, [[maybe_unused]] const QDBusObjectPath &objectPath)
{
    qInfo() << "Logind session removed for id:" << id;

    auto logindSession = m_systemSessions.value(id);
    if (!logindSession) {
        qWarning() << "LogindSession object unexpectedly not found for id:" << id;
        return;
    }
    untrackSession(id, logindSession);
}

void SeatManager::trackSession(const QString &id, const std::shared_ptr<LogindSession> &session)
{
    m_systemSessions.insert(id, session);

    if (session->isGreeter()) {
        auto seatName = session->seatName();
        if (session->seatName().isEmpty()) {
            qWarning() << "Greeter session has no associated seat name. Seems like a bug." << id;
            return;
        }

        if (auto seat = m_graphicalSeats.value(seatName); seat) {
            seat->setGreeterSession(session->objectPath());
            seat->activateExistingGreeter();
        }
    }
}

void SeatManager::untrackSession(const QString &id, const std::shared_ptr<LogindSession> &session)
{
    m_systemSessions.remove(id);

    if (session->isGreeter()) {
        auto seatName = session->seatName();
        if (session->seatName().isEmpty()) {
            qWarning() << "Greeter session has no associated seat name. Seems like a bug." << id;
            return;
        }

        if (auto seat = m_graphicalSeats.value(seatName); seat) {
            seat->setGreeterSession(std::nullopt);
        }
    }
}

void SeatManager::maybeSwitchToGreeter(LogindSeat *seat)
{
    if (!m_graphicalSeats.contains(seat->name())) {
        // Not a graphical seat so we don't switch on it anyway.
        return;
    }

    if (seat->hasActiveTTY()) {
        // Seat is already active
        return;
    }

    qDebug() << "Switching to greeter for seat:" << seat->name();
    switchToGreeter(seat->name());
}

} // namespace PLASMALOGIN

#include "SeatManager.moc"

#include "moc_SeatManager.cpp"
