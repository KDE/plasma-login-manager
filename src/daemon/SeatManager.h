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

#ifndef PLASMALOGIN_SEATMANAGER_H
#define PLASMALOGIN_SEATMANAGER_H

#include <QDBusObjectPath>
#include <QHash>
#include <QObject>
#include <QSet>

namespace PLASMALOGIN
{
class Seat;
class SessionBackend;

class SeatManager : public QObject
{
    Q_OBJECT
public:
    explicit SeatManager(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void initialize();
    void createSeat(const QString &name);
    void removeSeat(const QString &name);
    void switchToGreeter(const QString &seat);

    SessionBackend *backend() const;

Q_SIGNALS:
    void seatCreated(const QString &name);
    void seatRemoved(const QString &name);

private Q_SLOTS:
    void onSecureAttentionKey(const QString &name);
    void onSeatAdded(const QString &name, const QDBusObjectPath &objectPath);
    void onSeatRemoved(const QString &name, const QDBusObjectPath &objectPath);
    void onSeatCanGraphicalChanged(const QString &name, bool canGraphical);

private:
    QHash<QString, Seat *> m_seats;
    QSet<QString> m_pendingSeats;
    SessionBackend *m_backend = nullptr;
};
}

#endif // PLASMALOGIN_SEATMANAGER_H
