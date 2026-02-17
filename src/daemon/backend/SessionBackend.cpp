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

#include "SessionBackend.h"

#include <QDebug>

#if defined(HAVE_SYSTEMD) || defined(HAVE_ELOGIND)
#include "LogindBackend.h"
#endif

#if defined(HAVE_CONSOLEKIT2)
#include "ConsoleKit2Backend.h"
#endif

#include "FallbackBackend.h"

namespace PLASMALOGIN
{

SessionBackend::SessionBackend(QObject *parent)
    : QObject(parent)
{
}

SessionBackend::~SessionBackend()
{
}

SessionBackend *SessionBackend::create(QObject *parent)
{
#if defined(HAVE_SYSTEMD) || defined(HAVE_ELOGIND)
    auto logindBackend = new LogindBackend(parent);
    if (logindBackend->isAvailable()) {
        qDebug() << "Using logind/elogind session backend";
        return logindBackend;
    }
    delete logindBackend;
#endif

#if defined(HAVE_CONSOLEKIT2)
    auto ck2Backend = new ConsoleKit2Backend(parent);
    if (ck2Backend->isAvailable()) {
        qDebug() << "Using ConsoleKit2 session backend";
        return ck2Backend;
    }
    delete ck2Backend;
#endif

    qDebug() << "Using fallback session backend (single-seat only)";
    return new FallbackBackend(parent);
}

}
