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

#ifndef PLASMALOGIN_CONSOLEKIT2BACKEND_H
#define PLASMALOGIN_CONSOLEKIT2BACKEND_H

#include "SessionBackend.h"

#include <QDBusObjectPath>
#include <QHash>

class OrgFreedesktopConsoleKitManagerInterface;

namespace PLASMALOGIN
{

class ConsoleKit2SeatWatcher : public QObject
{
    Q_OBJECT
public:
    ConsoleKit2SeatWatcher(const QString &name, const QDBusObjectPath &objectPath, QObject *parent = nullptr);
    QString name() const;
    bool canGraphical() const;

Q_SIGNALS:
    void canGraphicalChanged(bool canGraphical);

private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    QString m_name;
    QDBusObjectPath m_path;
    bool m_canGraphical = false;
};

class ConsoleKit2Backend : public SessionBackend
{
    Q_OBJECT
public:
    explicit ConsoleKit2Backend(QObject *parent = nullptr);
    ~ConsoleKit2Backend() override;

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

private Q_SLOTS:
    void onSeatAdded(const QDBusObjectPath &path);
    void onSeatRemoved(const QDBusObjectPath &path);

private:
    QString seatNameFromPath(const QString &path);

    OrgFreedesktopConsoleKitManagerInterface *m_manager = nullptr;
    QHash<QString, ConsoleKit2SeatWatcher *> m_seatWatchers;
    bool m_available = false;
};

}

#endif // PLASMALOGIN_CONSOLEKIT2BACKEND_H
