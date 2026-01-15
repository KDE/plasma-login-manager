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

#include "XorgUserDisplayServer.h"
#include "Display.h"
#include "MainConfigLoader.h"
#include "Seat.h"

namespace PLASMALOGIN
{

QString XorgUserDisplayServer::command(Display *display)
{
    QStringList args;
    args << PlasmaLogin::config()->x11ServerPath() << PlasmaLogin::config()->x11ServerArguments().split(QLatin1Char(' '), Qt::SkipEmptyParts)
         << QStringLiteral("-background") << QStringLiteral("none") << QStringLiteral("-seat") << display->seat()->name() << QStringLiteral("-noreset")
         << QStringLiteral("-keeptty") << QStringLiteral("-novtswitch") << QStringLiteral("-verbose") << QStringLiteral("3");

    return args.join(QLatin1Char(' '));
}

} // namespace PLASMALOGIN
