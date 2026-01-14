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

#ifndef PLASMALOGIN_DAEMONAPP_H
#define PLASMALOGIN_DAEMONAPP_H

#include <QCoreApplication>
#include <QtCore>

#define daemonApp DaemonApp::instance()

namespace PLASMALOGIN
{
class Configuration;
class DisplayManager;
class SeatManager;

class DaemonApp : public QCoreApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(DaemonApp)
public:
    explicit DaemonApp(int &argc, char **argv);

    static DaemonApp *instance()
    {
        return self;
    }

    bool tryLockFirstLogin();

    QString hostName() const;
    DisplayManager *displayManager() const;
    SeatManager *seatManager() const;

public slots:
    int newSessionId();

private:
    static DaemonApp *self;

    int m_lastSessionId{0};

    bool m_firstloginLock{false};

    bool m_testing{false};
    DisplayManager *m_displayManager{nullptr};
    SeatManager *m_seatManager{nullptr};
};
}

#endif // PLASMALOGIN_DAEMONAPP_H
