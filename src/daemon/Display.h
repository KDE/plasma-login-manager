/***************************************************************************
 * SPDX-FileCopyrightText: 2014-2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 * SPDX-FileCopyrightText: 2014 Martin Bříza <mbriza@redhat.com>
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

#ifndef PLASMALOGIN_DISPLAY_H
#define PLASMALOGIN_DISPLAY_H

#include <QDir>
#include <QObject>
#include <QPointer>

#include "Auth.h"
#include "Session.h"

class QLocalSocket;

namespace PLASMALOGIN
{
class Authenticator;
class DisplayServer;
class Seat;
class SocketServer;
class Greeter;

class Display : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Display)
public:
    explicit Display(Seat *parent);
    ~Display() override;

    int terminalId() const;

    QString sessionType() const;
    QString reuseSessionId() const
    {
        return m_reuseSessionId;
    }

    Seat *seat() const;

public slots:
    virtual bool start();
    virtual void stop();

signals:
    void stopped();

    void loginFailed(QLocalSocket *socket);
    void loginSucceeded(QLocalSocket *socket);

protected:
    bool startAuth(const QString &user, const QString &password, const Session &session);
    virtual bool hasGreeter() const;

    bool m_started{false};

    int m_terminalId = -1;
    int m_sessionTerminalId = 0;

    QString m_passPhrase;
    QString m_sessionName;
    QString m_reuseSessionId;

    Auth *m_auth{nullptr};
    Seat *m_seat{nullptr};
    QPointer<QLocalSocket> m_socket;

protected slots:
    void slotRequestChanged();
    void slotAuthenticationFinished(const QString &user, bool success);
    void slotHelperFinished(Auth::HelperExitStatus status);
    virtual void slotSessionStarted(bool success);
    virtual void handleAutologinFailure();
};

class GreeterDisplay : public Display
{
    Q_OBJECT
public:
    explicit GreeterDisplay(Seat *parent);

public slots:
    bool start() override;
    void stop() override;

    void login(QLocalSocket *socket, const QString &user, const QString &password, const Session &session);
    void displayServerStarted();

private:
    void startSocketServerAndGreeter();
    bool hasGreeter() const override;

    SocketServer *m_socketServer{nullptr};
    Greeter *m_greeter{nullptr};

private slots:
    void slotAuthInfo(const QString &message, Auth::Info info);
    void slotAuthError(const QString &message, Auth::Error error);
    void slotSessionStarted(bool success) override;
};

class AutoLoginDisplay : public Display
{
    Q_OBJECT
public:
    explicit AutoLoginDisplay(Seat *parent);

    void setAutoLogin(const QString &user, const QString &session);

public slots:
    bool start() override;

protected slots:
    void handleAutologinFailure() override;

private:
    Session m_autologinSession;
    QString m_autologinUser;
};
}

#endif // PLASMALOGIN_DISPLAY_H
