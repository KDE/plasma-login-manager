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

#include "SocketServer.h"

#include "DaemonApp.h"
#include "Messages.h"
#include "SocketWriter.h"
#include "Utils.h"

#include <QLocalServer>
#include <QLocalSocket>

namespace PLASMALOGIN
{
SocketServer::SocketServer(QObject *parent)
    : QObject(parent)
{
}

QString SocketServer::socketAddress() const
{
    if (m_server)
        return m_server->fullServerName();
    return QString();
}

bool SocketServer::start(const QString &displayName)
{
    // check if the server has been created already
    if (m_server)
        return false;

    QString socketName = QStringLiteral("plasmalogin-%1-%2").arg(displayName).arg(generateName(6));

    // log message
    qDebug() << "Socket server starting...";

    // create server
    m_server = new QLocalServer(this);

    // set server options
    m_server->setSocketOptions(QLocalServer::UserAccessOption);

    // start listening
    if (!m_server->listen(socketName)) {
        // log message
        qCritical() << "Failed to start socket server.";

        // return fail
        return false;
    }

    // log message
    qDebug() << "Socket server started.";

    // connect signals
    connect(m_server, &QLocalServer::newConnection, this, &SocketServer::newConnection);

    // return success
    return true;
}

void SocketServer::stop()
{
    // check flag
    if (!m_server)
        return;

    // log message
    qDebug() << "Socket server stopping...";

    // delete server
    m_server->deleteLater();
    m_server = nullptr;

    // log message
    qDebug() << "Socket server stopped.";
}

void SocketServer::newConnection()
{
    // get pending connection
    QLocalSocket *socket = m_server->nextPendingConnection();

    // connect signals
    connect(socket, &QLocalSocket::readyRead, this, &SocketServer::readyRead);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void SocketServer::readyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());

    // check socket
    if (!socket)
        return;

    // input stream
    QDataStream input(socket);

    // Qt's QLocalSocket::readyRead is not designed to be called at every socket.write(),
    // so we need to use a loop to read all the signals.
    while (socket->bytesAvailable()) {
        // read message
        quint32 message;
        input >> message;

        switch (GreeterMessages(message)) {
        case GreeterMessages::Connect: {
            // log message
            qDebug() << "Message received from greeter: Connect";

            // send host name
            SocketWriter(socket) << quint32(DaemonMessages::HostName) << daemonApp->hostName();

            // emit signal
            emit connected();
        } break;
        case GreeterMessages::Login: {
            // log message
            qDebug() << "Message received from greeter: Login";

            // read username, pasword etc.
            QString user, password, filename;
            Session session;
            input >> user >> password >> session;

            // emit signal
            emit login(socket, user, password, session);
        } break;
        default: {
            // log message
            qWarning() << "Unknown message" << message;
        }
        }
    }
}

void SocketServer::loginFailed(QLocalSocket *socket)
{
    SocketWriter(socket) << quint32(DaemonMessages::LoginFailed);
}

void SocketServer::loginSucceeded(QLocalSocket *socket)
{
    SocketWriter(socket) << quint32(DaemonMessages::LoginSucceeded);
}

void SocketServer::informationMessage(QLocalSocket *socket, const QString &message)
{
    SocketWriter(socket) << quint32(DaemonMessages::InformationMessage) << message;
}
}
