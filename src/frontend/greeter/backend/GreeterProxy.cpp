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

#include "GreeterProxy.h"
#include "SocketWriter.h"

#include <QDebug>
#include <QLocalSocket>

namespace PLASMALOGIN
{
class GreeterProxyPrivate
{
public:
    SessionModel *sessionModel{nullptr};
    QLocalSocket *socket{nullptr};
};

GreeterProxy::GreeterProxy(QObject *parent)
    : QObject(parent)
    , d(new GreeterProxyPrivate())
{
    d->socket = new QLocalSocket(this);
    // connect signals
    connect(d->socket, &QLocalSocket::connected, this, &GreeterProxy::connected);
    connect(d->socket, &QLocalSocket::disconnected, this, &GreeterProxy::disconnected);
    connect(d->socket, &QLocalSocket::readyRead, this, &GreeterProxy::readyRead);
    connect(d->socket, &QLocalSocket::errorOccurred, this, &GreeterProxy::error);

    const QString socket = qEnvironmentVariable("SDDM_SOCKET");
    qDebug() << "TRYING SOCKET" << socket;
    // connect to server
    d->socket->connectToServer(socket);
}

GreeterProxy::~GreeterProxy()
{
    delete d;
}
void GreeterProxy::setSessionModel(SessionModel *model)
{
    d->sessionModel = model;
}

void GreeterProxy::login(const QString &user, const QString &password, const PLASMALOGIN::SessionType sessionType, const QString &sessionFileName) const
{
    SocketWriter(d->socket) << quint32(GreeterMessages::Login) << user << password << static_cast<uint32_t>(sessionType) << sessionFileName;
}

void GreeterProxy::connected()
{
    // log connection
    qDebug() << "Connected to the daemon.";

    // send connected message
    SocketWriter(d->socket) << quint32(GreeterMessages::Connect);
}

void GreeterProxy::disconnected()
{
    // log disconnection
    qDebug() << "Disconnected from the daemon.";

    Q_EMIT socketDisconnected();
}

void GreeterProxy::error()
{
    qCritical() << "Socket error: " << d->socket->errorString();
}

void GreeterProxy::readyRead()
{
    // input stream
    QDataStream input(d->socket);

    while (input.device()->bytesAvailable()) {
        // read message
        quint32 message;
        input >> message;

        switch (DaemonMessages(message)) {
        case DaemonMessages::LoginSucceeded: {
            // log message
            qDebug() << "Message received from daemon: LoginSucceeded";

            // emit signal
            emit loginSucceeded();
        } break;
        case DaemonMessages::LoginFailed: {
            // log message
            qDebug() << "Message received from daemon: LoginFailed";

            // emit signal
            emit loginFailed();
        } break;
        case DaemonMessages::InformationMessage: {
            QString message;
            input >> message;

            qDebug() << "Information Message received from daemon: " << message;
            emit informationMessage(message);
        } break;
        default: {
            // log message
            qWarning() << "Unknown message received from daemon.";
        }
        }
    }
}
}

#include "moc_GreeterProxy.cpp"
