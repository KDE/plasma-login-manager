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

class OrgFreedesktopLogin1ManagerInterface;

namespace PLASMALOGIN
{
class Seat;
class LogindSeat;
class LogindSession;

class SeatManager : public QObject
{
    Q_OBJECT
public:
    explicit SeatManager(QObject *parent = nullptr);

    void initialize();
    void createSeat(const QString &name);
    void removeSeat(const QString &name);
    void switchToGreeter(const QString &seat);

Q_SIGNALS:
    void seatCreated(const QString &name);
    void seatRemoved(const QString &name);

private Q_SLOTS:
    void logindSecureAttentionKey(const QString &name, const QDBusObjectPath &objectPath);
    void logindSeatAdded(const QString &name, const QDBusObjectPath &objectPath);
    void logindSeatRemoved(const QString &name, const QDBusObjectPath &objectPath);
    void logindSessionAdded(const QString &id, const QDBusObjectPath &objectPath);
    void logindSessionRemoved(const QString &id, const QDBusObjectPath &objectPath);

private:
    void trackSession(const QString &id, const std::shared_ptr<LogindSession> &session);
    void untrackSession(const QString &id, const std::shared_ptr<LogindSession> &session);

    //! Maybe switches to the greeter depending on the state of the removed session and the active seat
    void maybeSwitchToGreeter(LogindSeat *seat);

    OrgFreedesktopLogin1ManagerInterface *m_logind;

    //! Graphical Seat objects (effectively a heavier version of LogindSeat)
    QHash<QString, Seat *> m_graphicalSeats;

    //! All system seats. These seats are tracked for their graphical capabilities and when necessary a Seat gets created.
    QHash<QString, std::shared_ptr<LogindSeat>> m_systemSeats;

    /*! All system sessions.
        These are mostly only relevant for tieing to system seats.
        For practical reasons we track all sessions though.
    */
    QHash<QString, std::shared_ptr<LogindSession>> m_systemSessions;
};
}

#endif // PLASMALOGIN_SEATMANAGER_H
