/***************************************************************************
* SPDX-FileCopyrightText: 2021 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#ifndef PLASMALOGIN_WAYLANDDISPLAYSERVER_H
#define PLASMALOGIN_WAYLANDDISPLAYSERVER_H

#include "DisplayServer.h"

namespace PLASMALOGIN {

class WaylandDisplayServer : public DisplayServer
{
    Q_OBJECT
    Q_DISABLE_COPY(WaylandDisplayServer)
public:
    explicit WaylandDisplayServer(Display *parent);
    ~WaylandDisplayServer();

    QString sessionType() const;

    void setDisplayName(const QString &displayName);

public Q_SLOTS:
    bool start();
    void stop();
    void finished();
    void setupDisplay();
};

} // namespace PLASMALOGIN

#endif // PLASMALOGIN_WAYLANDDISPLAYSERVER_H
