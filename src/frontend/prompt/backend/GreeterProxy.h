/***************************************************************************
* Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
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

#ifndef SDDM_GREETERPROXY_H
#define SDDM_GREETERPROXY_H

#include <QObject>

class QLocalSocket;

namespace PLASMALOGIN {
    class SessionModel;

    class GreeterProxyPrivate;
    class GreeterProxy : public QObject {
        Q_OBJECT
        Q_DISABLE_COPY(GreeterProxy)

    public:
        explicit GreeterProxy(QObject *parent = 0);
        ~GreeterProxy();

        bool isConnected() const;

        void setSessionModel(SessionModel *model);

    public slots:
        void login(const QString &user, const QString &password, const int sessionIndex) const;

    private slots:
        void connected();
        void disconnected();
        void readyRead();
        void error();

    signals:
        void informationMessage(const QString &message);
        void hostNameChanged(const QString &hostName);

        void socketDisconnected();
        void loginFailed();
        void loginSucceeded();

    private:
        GreeterProxyPrivate *d { nullptr };
    };
}

#endif // SDDM_GREETERPROXY_H
