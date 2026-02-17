/***************************************************************************
 * SPDX-FileCopyrightText: 2024 Plasma Contributors
 * SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>
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

#include "LogindBackend.h"

#include "LogindDBusTypes.h"

#include <Login1Manager.h>
#include <Login1Seat.h>
#include <Login1Session.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

namespace PLASMALOGIN
{

LogindSeatWatcher::LogindSeatWatcher(const QString &name, const QDBusObjectPath &objectPath, QObject *parent)
    : QObject(parent)
    , m_name(name)
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
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
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
}

bool LogindSeatWatcher::canGraphical() const
{
    return m_canGraphical;
}

QString LogindSeatWatcher::name() const
{
    return m_name;
}

void LogindSeatWatcher::propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
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

LogindBackend::LogindBackend(QObject *parent)
    : SessionBackend(parent)
{
    if (QDBusConnection::systemBus().interface()->isServiceRegistered(Logind::serviceName())) {
        m_available = true;
        m_manager = new OrgFreedesktopLogin1ManagerInterface(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus(), this);
    }
}

LogindBackend::~LogindBackend()
{
    qDeleteAll(m_seatWatchers);
}

bool LogindBackend::isAvailable() const
{
    return m_available;
}

QList<SeatInfo> LogindBackend::listSeats()
{
    QList<SeatInfo> seats;
    if (!m_manager) {
        return seats;
    }

    QDBusPendingReply<NamedSeatPathList> reply = m_manager->ListSeats();
    reply.waitForFinished();

    if (!reply.isValid()) {
        return seats;
    }

    for (const NamedSeatPath &seat : reply.value()) {
        SeatInfo info;
        info.name = seat.name;
        info.path = seat.path.path();

        OrgFreedesktopLogin1SeatInterface seatIface(Logind::serviceName(), seat.path.path(), QDBusConnection::systemBus());
        info.canGraphical = seatIface.canGraphical();
        info.canTTY = seatIface.canTTY();

        seats.append(info);
    }

    return seats;
}

void LogindBackend::watchSeats()
{
    if (!m_manager) {
        return;
    }

    connect(m_manager, &OrgFreedesktopLogin1ManagerInterface::SeatNew, this, &LogindBackend::onSeatNew);
    connect(m_manager, &OrgFreedesktopLogin1ManagerInterface::SeatRemoved, this, &LogindBackend::onSeatRemoved);
    connect(m_manager, &OrgFreedesktopLogin1ManagerInterface::SecureAttentionKey, this, &LogindBackend::onSecureAttentionKey);
}

QList<BackendSessionInfo> LogindBackend::listSessions()
{
    QList<BackendSessionInfo> sessions;
    if (!m_manager) {
        return sessions;
    }

    QDBusPendingReply<SessionInfoList> reply = m_manager->ListSessions();
    reply.waitForFinished();

    if (!reply.isValid()) {
        return sessions;
    }

    for (const SessionInfo &s : reply.value()) {
        BackendSessionInfo info;
        info.sessionId = s.sessionId;
        info.userId = s.userId;
        info.userName = s.userName;
        info.seatId = s.seatId;
        info.sessionPath = s.sessionPath.path();

        OrgFreedesktopLogin1SessionInterface sessionIface(Logind::serviceName(), s.sessionPath.path(), QDBusConnection::systemBus());
        info.tty = sessionIface.tTY();
        info.state = sessionIface.state();
        info.service = sessionIface.service();
        info.desktop = sessionIface.desktop();
        info.vtNumber = sessionIface.vTNr();

        sessions.append(info);
    }

    return sessions;
}

QString LogindBackend::sessionTTY(const QString &sessionId)
{
    if (!m_manager) {
        return QString();
    }

    auto sessionPath = m_manager->GetSession(sessionId);
    sessionPath.waitForFinished();

    if (!sessionPath.isValid()) {
        return QString();
    }

    OrgFreedesktopLogin1SessionInterface sessionIface(Logind::serviceName(), sessionPath.value().path(), QDBusConnection::systemBus());
    return sessionIface.tTY();
}

QString LogindBackend::sessionState(const QString &sessionId)
{
    if (!m_manager) {
        return QString();
    }

    auto sessionPath = m_manager->GetSession(sessionId);
    sessionPath.waitForFinished();

    if (!sessionPath.isValid()) {
        return QString();
    }

    OrgFreedesktopLogin1SessionInterface sessionIface(Logind::serviceName(), sessionPath.value().path(), QDBusConnection::systemBus());
    return sessionIface.state();
}

bool LogindBackend::activateSession(const QString &sessionId)
{
    if (!m_manager) {
        return false;
    }

    auto reply = m_manager->ActivateSession(sessionId);
    reply.waitForFinished();
    return reply.isValid();
}

bool LogindBackend::unlockSession(const QString &sessionId)
{
    if (!m_manager) {
        return false;
    }

    auto reply = m_manager->UnlockSession(sessionId);
    reply.waitForFinished();
    return reply.isValid();
}

bool LogindBackend::canSeatTTY(const QString &seatName)
{
    if (!m_manager) {
        return seatName.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0;
    }

    auto seatPath = m_manager->GetSeat(seatName);
    seatPath.waitForFinished();

    if (!seatPath.isValid()) {
        return seatName.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0;
    }

    OrgFreedesktopLogin1SeatInterface seatIface(Logind::serviceName(), seatPath.value().path(), QDBusConnection::systemBus());
    if (seatIface.property("CanTTY").isValid()) {
        return seatIface.canTTY();
    }

    return seatName.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0;
}

int LogindBackend::sessionVTNumber(const QString &sessionId)
{
    if (!m_manager) {
        return -1;
    }

    auto sessionPath = m_manager->GetSession(sessionId);
    sessionPath.waitForFinished();

    if (!sessionPath.isValid()) {
        return -1;
    }

    OrgFreedesktopLogin1SessionInterface sessionIface(Logind::serviceName(), sessionPath.value().path(), QDBusConnection::systemBus());
    QString tty = sessionIface.tTY();
    if (tty.startsWith(QLatin1String("tty"))) {
        return QStringView(tty).mid(3).toInt();
    }
    return sessionIface.vTNr();
}

QDBusObjectPath LogindBackend::getSession(const QString &sessionId)
{
    if (!m_manager) {
        return QDBusObjectPath();
    }

    auto reply = m_manager->GetSession(sessionId);
    reply.waitForFinished();

    if (!reply.isValid()) {
        return QDBusObjectPath();
    }

    return reply.value();
}

bool LogindBackend::supportsSecureAttentionKey() const
{
    return true;
}

QString LogindBackend::backendName() const
{
#if defined(HAVE_ELOGIND)
    return QStringLiteral("elogind");
#else
    return QStringLiteral("systemd-logind");
#endif
}

void LogindBackend::onSeatNew(const QString &name, const QDBusObjectPath &path)
{
    auto watcher = new LogindSeatWatcher(name, path, this);
    m_seatWatchers.insert(name, watcher);

    connect(watcher, &LogindSeatWatcher::canGraphicalChanged, this, [this, watcher](bool canGraphical) {
        emit seatCanGraphicalChanged(watcher->name(), canGraphical);
    });

    emit seatAdded(name, path);
}

void LogindBackend::onSeatRemoved(const QString &name, const QDBusObjectPath &path)
{
    auto watcher = m_seatWatchers.take(name);
    delete watcher;
    emit seatRemoved(name, path);
}

void LogindBackend::onSecureAttentionKey(const QString &name, const QDBusObjectPath &path)
{
    Q_UNUSED(path);
    emit secureAttentionKey(name);
}

}

#include "LogindBackend.moc"
