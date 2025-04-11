/***************************************************************************
* SPDX-FileCopyrightText: 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#ifndef PLASMALOGIN_VIRTUALTERMINAL_H
#define PLASMALOGIN_VIRTUALTERMINAL_H

#include <QString>

namespace PLASMALOGIN {
    namespace VirtualTerminal {
        extern const char *defaultVtPath;

        QString path(int vt);
        int currentVt();
        int setUpNewVt();
        void jumpToVt(int vt, bool vt_auto);
    }
}

#endif // PLASMALOGIN_VIRTUALTERMINAL_H
