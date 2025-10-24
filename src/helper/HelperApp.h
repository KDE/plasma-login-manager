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
class PamBackend;
class UserSession;
class HelperApp : public QCoreApplication
{
    Q_OBJECT
public:
    HelperApp(int &argc, char **argv);
    virtual ~HelperApp();

    UserSession *session();
    const QString &user() const;

public slots:
    Request request(const Request &request);
    void info(const QString &message, Auth::Info type);
    void error(const QString &message, Auth::Error type);

private slots:
    void setUp();
    void doAuth();

private:
    QString m_user;
    qint64 m_id{-1};
    PamBackend *m_backend{nullptr};
    QLocalSocket *m_socket{nullptr};
};
}

#endif // Auth_H
