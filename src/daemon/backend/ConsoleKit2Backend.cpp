/***************************************************************************
 * SPDX-FileCopyrightText: 2024 Plasma Contributors
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

#include "ConsoleKit2Backend.h"

#include <ConsoleKitManager.h>
#include <ConsoleKitSeat.h>
#include <ConsoleKitSession.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

namespace PLASMALOGIN
{

static const QString CK2_SERVICE = QStringLiteral("org.freedesktop.ConsoleKit");
static const QString CK2_MANAGER_PATH = QStringLiteral("/org/freedesktop/ConsoleKit/Manager");
static const QString CK2_SEAT_IFACE = QStringLiteral("org.freedesktop.ConsoleKit.Seat");

ConsoleKit2SeatWatcher::ConsoleKit2SeatWatcher(const QString &name, const QDBusObjectPath &objectPath, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_path(objectPath)
    , m_canGraphical(false)
{
    QDBusConnection::systemBus().connect(CK2_SERVICE,
                                         objectPath.path(),
                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                         QStringLiteral("PropertiesChanged"),
                                         this,
                                         SLOT(propertiesChanged(QString, QVariantMap, QStringList)));

    auto canGraphicalMsg =
        QDBusMessage::createMethodCall(CK2_SERVICE, objectPath.path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    canGraphicalMsg << CK2_SEAT_IFACE << QStringLiteral("CanGraphical");

    QDBusPendingReply<QVariant> reply = QDBusConnection::systemBus().asyncCall(canGraphicalMsg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply, watcher]() {
        watcher->deleteLater();
        if (!reply.isValid()) {
            m_canGraphical = true;
            emit canGraphicalChanged(m_canGraphical);
            return;
        }

        bool value = reply.value().toBool();
        if (value != m_canGraphical) {
            m_canGraphical = value;
            emit canGraphicalChanged(m_canGraphical);
        }
    });
}

bool ConsoleKit2SeatWatcher::canGraphical() const
{
    return m_canGraphical;
}

QString ConsoleKit2SeatWatcher::name() const
{
    return m_name;
}

void ConsoleKit2SeatWatcher::propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);
    if (interface != CK2_SEAT_IFACE) {
        return;
    }

    if (changedProperties.contains(QStringLiteral("CanGraphical"))) {
        m_canGraphical = changedProperties[QStringLiteral("CanGraphical")].toBool();
        emit canGraphicalChanged(m_canGraphical);
    }
}

ConsoleKit2Backend::ConsoleKit2Backend(QObject *parent)
    : SessionBackend(parent)
{
    if (QDBusConnection::systemBus().interface()->isServiceRegistered(CK2_SERVICE)) {
        m_available = true;
        m_manager = new OrgFreedesktopConsoleKitManagerInterface(CK2_SERVICE, CK2_MANAGER_PATH, QDBusConnection::systemBus(), this);
    }
}

ConsoleKit2Backend::~ConsoleKit2Backend()
{
    qDeleteAll(m_seatWatchers);
}

bool ConsoleKit2Backend::isAvailable() const
{
    return m_available;
}

QList<SeatInfo> ConsoleKit2Backend::listSeats()
{
    QList<SeatInfo> seats;
    if (!m_manager) {
        return seats;
    }

    QDBusPendingReply<QList<QDBusObjectPath>> reply = m_manager->GetSeats();
    reply.waitForFinished();

    if (!reply.isValid()) {
        return seats;
    }

    for (const QDBusObjectPath &seatPath : reply.value()) {
        SeatInfo info;
        info.name = seatNameFromPath(seatPath.path());
        info.path = seatPath.path();

        OrgFreedesktopConsoleKitSeatInterface seatIface(CK2_SERVICE, seatPath.path(), QDBusConnection::systemBus());
        info.canGraphical = seatIface.canGraphical();
        info.canTTY = seatIface.canTTY();

        seats.append(info);
    }

    return seats;
}

void ConsoleKit2Backend::watchSeats()
{
    if (!m_manager) {
        return;
    }

    connect(m_manager, &OrgFreedesktopConsoleKitManagerInterface::SeatAdded, this, &ConsoleKit2Backend::onSeatAdded);
    connect(m_manager, &OrgFreedesktopConsoleKitManagerInterface::SeatRemoved, this, &ConsoleKit2Backend::onSeatRemoved);
}

QList<BackendSessionInfo> ConsoleKit2Backend::listSessions()
{
    QList<BackendSessionInfo> sessions;
    if (!m_manager) {
        return sessions;
    }

    QDBusPendingReply<QList<QDBusObjectPath>> reply = m_manager->GetSessions();
    reply.waitForFinished();

    if (!reply.isValid()) {
        return sessions;
    }

    for (const QDBusObjectPath &sessionPath : reply.value()) {
        OrgFreedesktopConsoleKitSessionInterface sessionIface(CK2_SERVICE, sessionPath.path(), QDBusConnection::systemBus());

        BackendSessionInfo info;
        info.sessionId = sessionIface.id();
        info.userId = sessionIface.unixUser();
        info.sessionPath = sessionPath.path();
        info.tty = sessionIface.display();
        info.state = sessionIface.sessionState();
        info.service = sessionIface.service();
        info.vtNumber = sessionIface.vTNr();

        auto seatReply = sessionIface.GetSeatId();
        seatReply.waitForFinished();
        if (seatReply.isValid()) {
            info.seatId = seatReply.value();
        }

        sessions.append(info);
    }

    return sessions;
}

QString ConsoleKit2Backend::sessionTTY(const QString &sessionId)
{
    if (!m_manager) {
        return QString();
    }

    auto sessionPath = m_manager->GetSessionForUnixProcess(QCoreApplication::applicationPid());
    sessionPath.waitForFinished();

    if (!sessionPath.isValid()) {
        return QString();
    }

    OrgFreedesktopConsoleKitSessionInterface sessionIface(CK2_SERVICE, sessionPath.value().path(), QDBusConnection::systemBus());
    return sessionIface.display();
}

QString ConsoleKit2Backend::sessionState(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return QString();
}

bool ConsoleKit2Backend::activateSession(const QString &sessionId)
{
    if (!m_manager) {
        return false;
    }

    const auto sessions = listSessions();
    for (const BackendSessionInfo &s : sessions) {
        if (s.sessionId == sessionId) {
            OrgFreedesktopConsoleKitSessionInterface sessionIface(CK2_SERVICE, s.sessionPath, QDBusConnection::systemBus());
            auto reply = sessionIface.Activate();
            reply.waitForFinished();
            return reply.isValid();
        }
    }

    return false;
}

bool ConsoleKit2Backend::unlockSession(const QString &sessionId)
{
    if (!m_manager) {
        return false;
    }

    const auto sessions = listSessions();
    for (const BackendSessionInfo &s : sessions) {
        if (s.sessionId == sessionId) {
            OrgFreedesktopConsoleKitSessionInterface sessionIface(CK2_SERVICE, s.sessionPath, QDBusConnection::systemBus());
            auto reply = sessionIface.Unlock();
            reply.waitForFinished();
            return reply.isValid();
        }
    }

    return false;
}

bool ConsoleKit2Backend::canSeatTTY(const QString &seatName)
{
    if (!m_manager) {
        return seatName.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0;
    }

    const auto seats = listSeats();
    for (const SeatInfo &s : seats) {
        if (s.name == seatName) {
            return s.canTTY;
        }
    }

    return seatName.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0;
}

int ConsoleKit2Backend::sessionVTNumber(const QString &sessionId)
{
    const auto sessions = listSessions();
    for (const BackendSessionInfo &s : sessions) {
        if (s.sessionId == sessionId) {
            return s.vtNumber;
        }
    }
    return -1;
}

QDBusObjectPath ConsoleKit2Backend::getSession(const QString &sessionId)
{
    const auto sessions = listSessions();
    for (const BackendSessionInfo &s : sessions) {
        if (s.sessionId == sessionId) {
            return QDBusObjectPath(s.sessionPath);
        }
    }
    return QDBusObjectPath();
}

QString ConsoleKit2Backend::backendName() const
{
    return QStringLiteral("ConsoleKit2");
}

void ConsoleKit2Backend::onSeatAdded(const QDBusObjectPath &path)
{
    QString name = seatNameFromPath(path.path());
    auto watcher = new ConsoleKit2SeatWatcher(name, path, this);
    m_seatWatchers.insert(name, watcher);

    connect(watcher, &ConsoleKit2SeatWatcher::canGraphicalChanged, this, [this, watcher](bool canGraphical) {
        emit seatCanGraphicalChanged(watcher->name(), canGraphical);
    });

    emit seatAdded(name, path);
}

void ConsoleKit2Backend::onSeatRemoved(const QDBusObjectPath &path)
{
    QString name = seatNameFromPath(path.path());
    auto watcher = m_seatWatchers.take(name);
    delete watcher;
    emit seatRemoved(name, path);
}

QString ConsoleKit2Backend::seatNameFromPath(const QString &path)
{
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    if (lastSlash >= 0) {
        return path.mid(lastSlash + 1);
    }
    return path;
}

}

#include "ConsoleKit2Backend.moc"
