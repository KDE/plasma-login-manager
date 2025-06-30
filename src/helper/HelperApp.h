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

    /*!
     \brief Write utmp/wtmp/btmp records when a user logs in
     \param vt  Virtual terminal (tty7, tty8,...)
     \param displayName  Display (:0, :1,...)
     \param user  User logging in
     \param pid  User process ID (e.g. PID of startkde)
     \param authSuccessful  Was authentication successful
    */
    void utmpLogin(const QString &vt, const QString &displayName, const QString &user, qint64 pid, bool authSuccessful);

    /*!
     \brief Write utmp/wtmp records when a user logs out
     \param vt  Virtual terminal (tty7, tty8,...)
     \param displayName  Display (:0, :1,...)
     \param pid  User process ID (e.g. PID of startkde)
    */
    void utmpLogout(const QString &vt, const QString &displayName, qint64 pid);
};
}

#endif // Auth_H
