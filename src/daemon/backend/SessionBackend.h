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

#ifndef PLASMALOGIN_SESSIONBACKEND_H
#define PLASMALOGIN_SESSIONBACKEND_H

#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include <QString>

namespace PLASMALOGIN
{

struct SeatInfo {
    QString name;
    QString path;
    bool canGraphical = false;
    bool canTTY = false;
};

struct BackendSessionInfo {
    QString sessionId;
    uint userId = 0;
    QString userName;
    QString seatId;
    QString sessionPath;
    QString tty;
    QString state;
    QString service;
    QString desktop;
    int vtNumber = 0;
};

class SessionBackend : public QObject
{
    Q_OBJECT
public:
    explicit SessionBackend(QObject *parent = nullptr);
    ~SessionBackend() override;

    static SessionBackend *create(QObject *parent = nullptr);

    virtual bool isAvailable() const = 0;

    virtual QList<SeatInfo> listSeats() = 0;

    virtual void watchSeats() = 0;

    virtual QList<BackendSessionInfo> listSessions() = 0;

    virtual QString sessionTTY(const QString &sessionId) = 0;

    virtual QString sessionState(const QString &sessionId) = 0;

    virtual bool activateSession(const QString &sessionId) = 0;

    virtual bool unlockSession(const QString &sessionId) = 0;

    virtual bool canSeatTTY(const QString &seatName) = 0;

    virtual int sessionVTNumber(const QString &sessionId) = 0;

    virtual QDBusObjectPath getSession(const QString &sessionId) = 0;

    virtual bool supportsSecureAttentionKey() const
    {
        return false;
    }

    virtual QString backendName() const = 0;

Q_SIGNALS:
    void seatAdded(const QString &name, const QDBusObjectPath &path);
    void seatRemoved(const QString &name, const QDBusObjectPath &path);
    void seatCanGraphicalChanged(const QString &name, bool canGraphical);
    void secureAttentionKey(const QString &seatName);
};

}

#endif // PLASMALOGIN_SESSIONBACKEND_H
