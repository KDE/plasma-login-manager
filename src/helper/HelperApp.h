/*
 * Main authentication application class
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef Auth_H
#define Auth_H

#include <QtCore/QCoreApplication>
#include <QtCore/QProcessEnvironment>

#include "AuthMessages.h"

class QLocalSocket;

namespace PLASMALOGIN
{
class Backend;
class UserSession;
class HelperApp : public QCoreApplication
{
    Q_OBJECT
public:
    HelperApp(int &argc, char **argv);
    virtual ~HelperApp();

    UserSession *session();
    const QString &user() const;
    const QByteArray &cookie() const;

public slots:
    Request request(const Request &request);
    void info(const QString &message, Auth::Info type);
    void error(const QString &message, Auth::Error type);
    QProcessEnvironment authenticated(const QString &user);
    void displayServerStarted(const QString &displayName);
    void sessionOpened(bool success);

private slots:
    void setUp();
    void doAuth();

    void sessionFinished(int status);

private:
    qint64 m_id{-1};
    Backend *m_backend{nullptr};
    UserSession *m_session{nullptr};
    QLocalSocket *m_socket{nullptr};
    QString m_user{};
    // TODO: get rid of this in a nice clean way along the way with moving to user session X server
    QByteArray m_cookie{};

};
}

#endif // Auth_H
