/***************************************************************************
* SPDX-FileCopyrightText: 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#ifndef PLASMALOGIN_SOCKETWRITER_H
#define PLASMALOGIN_SOCKETWRITER_H

#include <QDataStream>
#include <QLocalSocket>

#include "Session.h"

namespace PLASMALOGIN {
    class SocketWriter {
        Q_DISABLE_COPY(SocketWriter)
    public:
        SocketWriter(QLocalSocket *socket);
        ~SocketWriter();

        SocketWriter &operator << (const quint32 &u);
        SocketWriter &operator << (const QString &s);
        SocketWriter &operator << (const Session &s);

    private:
        QByteArray data;
        QDataStream *output;
        QLocalSocket *socket;
    };
}

#endif // PLASMALOGIN_SOCKETWRITER_H
