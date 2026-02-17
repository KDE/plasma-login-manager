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

#include "backend/SessionBackend.h"

namespace PLASMALOGIN
{

void SeatManager::initialize()
{
    m_backend = SessionBackend::create(this);

    if (!m_backend->isAvailable() || m_backend->backendName() == QLatin1String("fallback")) {
        createSeat(QStringLiteral("seat0"));

        if (m_backend->backendName() != QLatin1String("fallback")) {
            delete m_backend;
            m_backend = nullptr;
        }
        return;
    }

    connect(m_backend, &SessionBackend::seatAdded, this, &SeatManager::onSeatAdded);
    connect(m_backend, &SessionBackend::seatRemoved, this, &SeatManager::onSeatRemoved);
    connect(m_backend, &SessionBackend::seatCanGraphicalChanged, this, &SeatManager::onSeatCanGraphicalChanged);
    connect(m_backend, &SessionBackend::secureAttentionKey, this, &SeatManager::onSecureAttentionKey);

    m_backend->watchSeats();

    for (const SeatInfo &seat : m_backend->listSeats()) {
        onSeatAdded(seat.name, QDBusObjectPath(seat.path));
        if (seat.canGraphical) {
            onSeatCanGraphicalChanged(seat.name, true);
        }
    }
}

SessionBackend *SeatManager::backend() const
{
    return m_backend;
}

void SeatManager::createSeat(const QString &name)
{
    if (m_seats.contains(name)) {
        return;
    }

    Seat *seat = new Seat(name, this);
    m_seats.insert(name, seat);
    emit seatCreated(name);
}

void SeatManager::removeSeat(const QString &name)
{
    if (!m_seats.contains(name)) {
        return;
    }

    Seat *seat = m_seats.take(name);
    seat->deleteLater();
    emit seatRemoved(name);
}

void SeatManager::switchToGreeter(const QString &name)
{
    if (!m_seats.contains(name)) {
        return;
    }

    if (m_backend && m_backend->isAvailable()) {
        const auto sessions = m_backend->listSessions();
        for (const BackendSessionInfo &s : sessions) {
            if (s.userName == QLatin1String("plasmalogin") && s.service == QLatin1String("plasmalogin-greeter") && s.seatId == name) {
                m_backend->activateSession(s.sessionId);
                return;
            }
        }
    }

    m_seats.value(name)->createDisplay();
}

void SeatManager::onSecureAttentionKey(const QString &name)
{
    daemonApp->seatManager()->switchToGreeter(name);
}

void SeatManager::onSeatAdded(const QString &name, const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath);
    m_pendingSeats.insert(name);
}

void SeatManager::onSeatRemoved(const QString &name, const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath);
    m_pendingSeats.remove(name);
    removeSeat(name);
}

void SeatManager::onSeatCanGraphicalChanged(const QString &name, bool canGraphical)
{
    if (canGraphical) {
        if (m_pendingSeats.contains(name)) {
            m_pendingSeats.remove(name);
            createSeat(name);
        }
    } else {
        removeSeat(name);
    }
}

}

#include "moc_SeatManager.cpp"
