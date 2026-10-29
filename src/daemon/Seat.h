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

#ifndef PLASMALOGIN_SEAT_H
#define PLASMALOGIN_SEAT_H

#include "Display.h"
#include <QObject>
#include <QVector>

#include "GreeterSession.h"

namespace PLASMALOGIN
{
class Display;

class Seat : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Seat)
public:
    explicit Seat(const QString &name, QObject *parent = 0);

    const QString &name() const;
    [[nodiscard]] Display *createDisplay();
    bool canTTY();
    bool tryLockFirstLogin();
    VirtualTerminal::Terminal availableVt() const;
    QString reusableSessionId(const QString &user) const;
    void activateSession(const QString &sessionId) const;
    void unlockAndActivateSession(const QString &sessionId) const;
    bool activateExistingGreeter();
    void switchToGreeter();
    void setGreeterSession(const std::optional<QDBusObjectPath> &objectPath);

private slots:
    void displayStopped();

private:
    bool isTtyInUse(const QString &tty) const;
    void maybeSetupAutoLogin(Display *display);

    QString m_name;

    bool m_firstLoginLock = false;

    /*!
        Legacy tech. A Display is effectively either a Greeter session or a User session. Unfortunately this complicates
        matters a bit because a Display may be "stopped" (have no Greeter) but still be active in so far as a session is managed by it.
    */
    QVector<Display *> m_displays;
    std::optional<GreeterSession> m_greeterSession;
};
}

#endif // PLASMALOGIN_SEAT_H
