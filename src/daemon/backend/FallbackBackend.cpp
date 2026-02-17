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

#include "FallbackBackend.h"

#include <QDebug>
#include <unistd.h>

#include "VirtualTerminal.h"

namespace PLASMALOGIN
{

FallbackBackend::FallbackBackend(QObject *parent)
    : SessionBackend(parent)
{
    qDebug() << "Fallback session backend initialized (no session manager available)";
}

FallbackBackend::~FallbackBackend()
{
}

bool FallbackBackend::isAvailable() const
{
    return true;
}

QList<SeatInfo> FallbackBackend::listSeats()
{
    SeatInfo seat0;
    seat0.name = QStringLiteral("seat0");
    seat0.path = QString();
    seat0.canGraphical = true;
    seat0.canTTY = access(VirtualTerminal::defaultVtPath, F_OK) == 0;
    return {seat0};
}

void FallbackBackend::watchSeats()
{
}

QList<BackendSessionInfo> FallbackBackend::listSessions()
{
    return {};
}

QString FallbackBackend::sessionTTY(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return QString();
}

QString FallbackBackend::sessionState(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return QString();
}

bool FallbackBackend::activateSession(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return false;
}

bool FallbackBackend::unlockSession(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return false;
}

bool FallbackBackend::canSeatTTY(const QString &seatName)
{
    if (seatName.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0) {
        return access(VirtualTerminal::defaultVtPath, F_OK) == 0;
    }
    return false;
}

int FallbackBackend::sessionVTNumber(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return -1;
}

QDBusObjectPath FallbackBackend::getSession(const QString &sessionId)
{
    Q_UNUSED(sessionId);
    return QDBusObjectPath();
}

QString FallbackBackend::backendName() const
{
    return QStringLiteral("fallback");
}

}
