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

#ifndef PLASMALOGIN_SOCKETSERVER_H
#define PLASMALOGIN_SOCKETSERVER_H

#include <QObject>
#include <QString>

#include "Session.h"

class QLocalServer;
class QLocalSocket;

namespace PLASMALOGIN {
    class SocketServer : public QObject {
        Q_OBJECT
        Q_DISABLE_COPY(SocketServer)
    public:
        explicit SocketServer(QObject *parent = 0);

        bool start(const QString &plasmaloginName);
        void stop();

        QString socketAddress() const;

    private slots:
        void newConnection();
        void readyRead();

    public slots:
        void informationMessage(QLocalSocket *socket, const QString &message);
        void loginFailed(QLocalSocket *socket);
        void loginSucceeded(QLocalSocket *socket);

    signals:
        void login(QLocalSocket *socket,
                   const QString &user, const QString &password,
                   const Session &session);
        void connected();

    private:
        QLocalServer *m_server { nullptr };
    };
}

#endif // PLASMALOGIN_SOCKETSERVER_H
