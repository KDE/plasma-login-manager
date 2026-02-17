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

#ifndef PLASMALOGIN_FALLBACKBACKEND_H
#define PLASMALOGIN_FALLBACKBACKEND_H

#include "SessionBackend.h"

namespace PLASMALOGIN
{

class FallbackBackend : public SessionBackend
{
    Q_OBJECT
public:
    explicit FallbackBackend(QObject *parent = nullptr);
    ~FallbackBackend() override;

    bool isAvailable() const override;

    QList<SeatInfo> listSeats() override;
    void watchSeats() override;
    QList<BackendSessionInfo> listSessions() override;
    QString sessionTTY(const QString &sessionId) override;
    QString sessionState(const QString &sessionId) override;
    bool activateSession(const QString &sessionId) override;
    bool unlockSession(const QString &sessionId) override;
    bool canSeatTTY(const QString &seatName) override;
    int sessionVTNumber(const QString &sessionId) override;
    QDBusObjectPath getSession(const QString &sessionId) override;

    QString backendName() const override;
};

}

#endif // PLASMALOGIN_FALLBACKBACKEND_H
