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

#ifndef PLASMALOGIN_LOGINDBACKEND_H
#define PLASMALOGIN_LOGINDBACKEND_H

#include "SessionBackend.h"

#include <QDBusObjectPath>
#include <QHash>

class OrgFreedesktopLogin1ManagerInterface;

namespace PLASMALOGIN
{

class LogindSeatWatcher : public QObject
{
    Q_OBJECT
public:
    LogindSeatWatcher(const QString &name, const QDBusObjectPath &objectPath, QObject *parent = nullptr);
    QString name() const;
    bool canGraphical() const;

Q_SIGNALS:
    void canGraphicalChanged(bool canGraphical);

private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    QString m_name;
    bool m_canGraphical = false;
};

class LogindBackend : public SessionBackend
{
    Q_OBJECT
public:
    explicit LogindBackend(QObject *parent = nullptr);
    ~LogindBackend() override;

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

    bool supportsSecureAttentionKey() const override;
    QString backendName() const override;

private Q_SLOTS:
    void onSeatNew(const QString &name, const QDBusObjectPath &path);
    void onSeatRemoved(const QString &name, const QDBusObjectPath &path);
    void onSecureAttentionKey(const QString &name, const QDBusObjectPath &path);

private:
    OrgFreedesktopLogin1ManagerInterface *m_manager = nullptr;
    QHash<QString, LogindSeatWatcher *> m_seatWatchers;
    bool m_available = false;
};

}

#endif // PLASMALOGIN_LOGINDBACKEND_H
