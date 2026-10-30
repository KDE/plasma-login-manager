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

    [[nodiscard]] QString activeSession() const
    {
        return m_activeSession;
    }

Q_SIGNALS:
    void canGraphicalChanged(bool canGraphical);
    void activeSessionChanged();

private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void updateActiveSession(const NamedSessionPath &activeSession);

    QString m_name;
    bool m_canGraphical = false;
    QString m_activeSession;
};

class LogindSession
{
public:
    enum class Type {
        Unknown,
        Unspecified,
        TTY,
        X11,
        Mir,
        Wayland,
    };

    LogindSession(const QDBusObjectPath &objectPath, const QString &seatName = QString());
    [[nodiscard]] QString id() const
    {
        return m_id;
    }
    QString seatName() const;
    [[nodiscard]] QDBusObjectPath objectPath() const
    {
        return m_objectPath;
    }
    [[nodiscard]] bool isGreeter() const
    {
        return m_isGreeter;
    }

    [[nodiscard]] static bool isGreeter(const OrgFreedesktopLogin1SessionInterface &sessionInterface)
    {
        auto userName = sessionInterface.name(); // the Name property is indeed the user's name
        auto service = sessionInterface.service();
        return userName == "plasmalogin"_L1 && service == "plasmalogin-greeter"_L1;
    }
    [[nodiscard]] uint vtNr() const
    {
        return m_vtNr;
    }
    [[nodiscard]] bool isGraphical() const
    {
        switch (m_type) {
        case LogindSession::Type::TTY:
        case LogindSession::Type::Unspecified:
        case LogindSession::Type::Unknown:
            return false;
        case LogindSession::Type::Mir:
        case LogindSession::Type::Wayland:
        case LogindSession::Type::X11:
            break;
        }
        // Default to true so future session types, which are more likely to be graphical, are handled correctly.
        return true;
    }

private:
    QString m_seatName;
    uint m_vtNr = 0;
    QString m_id;
    QDBusObjectPath m_objectPath;
    bool m_isGreeter = false;
    Type m_type = Type::Unknown;
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
        // There is no active session right now. Let's figure out what to do.
        m_activeSession.clear();
        Q_EMIT activeSessionChanged();
        return;
    }

    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), activeSessionPath, QDBusConnection::systemBus());
    m_activeSession = activeSession.name;
    Q_EMIT activeSessionChanged();
}

LogindSession::LogindSession(const QDBusObjectPath &objectPath, const QString &seatName)
    : m_seatName(seatName)
    , m_objectPath(objectPath)
{
    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), objectPath.path(), QDBusConnection::systemBus());
    if (m_seatName.isEmpty()) {
        m_seatName = session.seat().name;
    }

    m_id = session.id();
    m_vtNr = session.vTNr();
    m_type = [&] {
        if (session.type() == "wayland"_L1) {
            return Type::Wayland;
        }
        if (session.type() == "tty"_L1) {
            return Type::TTY;
        }
        if (session.type() == "unspecified"_L1) {
            return Type::Unspecified;
        }
        if (session.type() == "x11"_L1) {
            return Type::X11;
        }
        if (session.type() == "mir"_L1) {
            return Type::Mir;
        }
        qWarning() << "Unknown login1 session type:" << session.type();
        return Type::Unknown;
    }();

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

std::optional<GreeterSession> SeatManager::greeterSessionForSeat(const QString &seatName)
{
    for (const auto &[id, session] : m_systemSessions.asKeyValueRange()) {
        if (session->seatName() == seatName && session->isGreeter()) {
            return std::make_optional<GreeterSession>(session->objectPath());
        }
    }
    return std::nullopt;
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
    connect(logindSeat.get(), &LogindSeat::activeSessionChanged, this, [this, logindSeat = logindSeat.get() /* don't hold a shared_ptr! */]() {
        if (auto seat = m_graphicalSeats.value(logindSeat->name()); seat) {
            maybeSwitchOnSessionActive(seat);
        }
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
    qInfo() << "Tracking session with id:" << id;
    m_systemSessions.insert(id, session);

    auto seatName = session->seatName();
    if (session->seatName().isEmpty()) {
        qInfo() << "Added session has no associated seat name. Will not manage it." << id;
        return;
    }

    if (auto seat = m_graphicalSeats.value(seatName); seat) {
        maybeSwitchOnSessionAdd(seat, session);
    }
}

void SeatManager::untrackSession(const QString &id, const std::shared_ptr<LogindSession> &session)
{
    m_systemSessions.remove(id);

    auto seatName = session->seatName();
    if (session->seatName().isEmpty()) {
        qInfo() << "Removed session has no associated seat name. Will not manage it." << id;
        return;
    }

    if (auto seat = m_graphicalSeats.value(seatName); seat) {
        maybeSwitchOnSessionRemove(seat, session);
    }
}

void SeatManager::maybeSwitchOnSessionAdd(Seat *graphicalSeat, const std::shared_ptr<LogindSession> &session)
{
    qInfo() << Q_FUNC_INFO;

    auto seat = m_systemSeats.value(graphicalSeat->name(), nullptr);
    if (!seat) {
        qWarning() << "No system seat found for graphical seat:" << graphicalSeat->name() << "This is a bug!";
        return;
    }

    {
        auto sessionsForSeat = m_systemSessions.asKeyValueRange() | std::views::filter([seat](const auto &pair) {
                                   auto &&[sessionName, session] = pair;
                                   return session->seatName() == seat->name();
                               });

        auto sessionNames = sessionsForSeat | std::views::keys;
        const auto count = std::ranges::distance(sessionNames);
        if (count == 0) {
            qWarning() << "No sessions for graphical seat:" << graphicalSeat->name() << "This is a bug considering we are currently adding!";
            return;
        }

        if (count == 1) {
            qDebug() << "Only one session for graphical seat:" << graphicalSeat->name() << "Switching to it";
            graphicalSeat->activateSession(sessionNames.front());
            return;
        }
    }

    // We have more than one session if we reached this point.

    if (session->isGreeter()) {
        qDebug() << "Added session is a greeter for graphical seat:" << graphicalSeat->name();
        graphicalSeat->activateSession(session->id());
        return;
    }

    if (session->isGraphical()) {
        qDebug() << "Added session is a graphical session for graphical seat:" << graphicalSeat->name() << ". Discarding greeter.";
        discardGreeterForSeat(graphicalSeat->name());
        graphicalSeat->activateSession(session->id());
        return;
    }

    qDebug() << "Don't know what to do. Staying on current VT";
}

void SeatManager::maybeSwitchOnSessionRemove(Seat *graphicalSeat, const std::shared_ptr<LogindSession> &session)
{
    qInfo() << Q_FUNC_INFO;

    auto seat = m_systemSeats.value(graphicalSeat->name(), nullptr);
    if (!seat) {
        qWarning() << "No system seat found for graphical seat:" << graphicalSeat->name() << "This is a bug!";
        return;
    }

    auto wasActiveVT = session->vtNr() == VirtualTerminal::currentVt();
    if (!wasActiveVT) {
        qWarning() << "Session is not on the active VT for graphical seat:" << graphicalSeat->name();
        return;
    }

    {
        auto graphicalSessionsForSeat = m_systemSessions.asKeyValueRange() | std::views::filter([seat](const auto &pair) {
                                            auto &&[sessionName, session] = pair;
                                            return session->isGraphical() && session->seatName() == seat->name();
                                        });
        const auto count = std::ranges::distance(graphicalSessionsForSeat);
        if (count == 0) {
            qDebug() << "We've run out of sessions. Firing up a new greeter.";
            switchToGreeter(seat->name());
            return;
        }
    }

    // We have at least one session if we reached this point.

    if (session->isGreeter()) {
        qDebug() << "Removed session was a greeter for graphical seat:" << graphicalSeat->name();
        return;
    }

    qDebug() << "Removed session was a user session for graphical seat:" << graphicalSeat->name() << ". Switching to greeter..";
    switchToGreeter(seat->name());
}

void SeatManager::maybeSwitchOnSessionActive(Seat *graphicalSeat)
{
    qInfo() << Q_FUNC_INFO;

    auto seat = m_systemSeats.value(graphicalSeat->name(), nullptr);
    if (!seat) {
        qWarning() << "No system seat found for graphical seat:" << graphicalSeat->name() << "This is a bug!";
        return;
    }

    auto session = m_systemSessions.value(seat->activeSession(), nullptr);
    if (session) {
        if (session->isGreeter()) {
            qDebug() << "Active session for graphical seat:" << graphicalSeat->name() << "is a greeter.";
            return;
        }

        if (session->isGraphical()) {
            qDebug() << "Active session for graphical seat:" << graphicalSeat->name() << "is a graphical session. Discarding greeter.";
            discardGreeterForSeat(seat->name());
            return;
        }
        qInfo() << "Active session for graphical seat:" << graphicalSeat->name() << "is not a graphical session. Keeping greeter.";
    }

    // No **graphical** session is currently active

    // On startup let's jump to the greeter immediately. This ensures we don't end up in a weird scenario where a tty
    // user doesn't know which VT the greeter happens to appear on.
    if (static auto initialJump = false; !initialJump) {
        initialJump = true;
        switchToGreeter(seat->name());
        return;
    }

    qDebug() << "Don't know what to do. Staying on current VT";
}

void SeatManager::discardGreeterForSeat(const QString &seatName)
{
    for (const auto &[id, session] : m_systemSessions.asKeyValueRange()) {
        if (session->seatName() == seatName && session->isGreeter()) {
            qDebug() << "Discarding greeter session:" << id << "for seat:" << seatName;
            // Let's assume we know what we are doing and sigterm will always result in actual timely termination :O
            GreeterSession(session->objectPath()).sigterm();
        }
    }
}

} // namespace PLASMALOGIN

#include "SeatManager.moc"

#include "moc_SeatManager.cpp"
