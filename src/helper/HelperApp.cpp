/*
 * Main authentication application class
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "HelperApp.h"
#include "Configuration.h"
#include "SafeDataStream.h"
#include "UserSession.h"
#include "backend/PamBackend.h"

#include "MessageHandler.h"
#include "SignalHandler.h"
#include "VirtualTerminal.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtNetwork/QLocalSocket>

#include <iostream>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <QByteArray>
#include <signal.h>

namespace PLASMALOGIN
{
HelperApp::HelperApp(int &argc, char **argv)
    : QCoreApplication(argc, argv)
    , m_backend(new PamBackend(this))
    , m_session(new UserSession(this))
    , m_socket(new QLocalSocket(this))
{
    qInstallMessageHandler(HelperMessageHandler);
    SignalHandler *s = new SignalHandler(this);
    QObject::connect(s, &SignalHandler::sigtermReceived, m_session, [] {
        QCoreApplication::instance()->exit(-1);
    });

    QTimer::singleShot(0, this, SLOT(setUp()));
}

void HelperApp::setUp()
{
    const QStringList args = QCoreApplication::arguments();
    QString server;
    int pos;

    if ((pos = args.indexOf(QStringLiteral("--socket"))) >= 0) {
        if (pos >= args.length() - 1) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }
        server = args[pos + 1];
    }

    if ((pos = args.indexOf(QStringLiteral("--id"))) >= 0) {
        if (pos >= args.length() - 1) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }
        m_id = QString(args[pos + 1]).toLongLong();
    }

    if ((pos = args.indexOf(QStringLiteral("--start"))) >= 0) {
        if (pos >= args.length() - 1) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }
        m_session->setPath(args[pos + 1]);
    }

    if ((pos = args.indexOf(QStringLiteral("--user"))) >= 0) {
        if (pos >= args.length() - 1) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }
        m_user = args[pos + 1];
    }

    if ((pos = args.indexOf(QStringLiteral("--display-server"))) >= 0) {
        if (pos >= args.length() - 1) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }
        m_session->setDisplayServerCommand(args[pos + 1]);
        m_backend->setDisplayServer(true);
    }

    if ((pos = args.indexOf(QStringLiteral("--autologin"))) >= 0) {
        m_backend->setAutologin(true);
    }

    if ((pos = args.indexOf(QStringLiteral("--greeter"))) >= 0) {
        m_backend->setGreeter(true);
    }

    if (server.isEmpty() || m_id <= 0) {
        qCritical() << "This application is not supposed to be executed manually";
        exit(Auth::HELPER_OTHER_ERROR);
        return;
    }

    connect(m_socket, &QLocalSocket::connected, this, &HelperApp::doAuth);
    connect(m_session, &UserSession::finished, this, &HelperApp::sessionFinished);
    m_socket->connectToServer(server, QIODevice::ReadWrite | QIODevice::Unbuffered);
}

void HelperApp::doAuth()
{
    SafeDataStream str(m_socket);
    str << Msg::HELLO << m_id;
    str.send();
    if (str.status() != QDataStream::Ok) {
        qCritical() << "Couldn't write initial message:" << str.status();
    }

    if (!m_backend->start(m_user)) {
        authenticated(QString());
        exit(Auth::HELPER_AUTH_ERROR);
        return;
    }

    Q_ASSERT(getuid() == 0);
    if (!m_backend->authenticate()) {
        authenticated(QString());
        exit(Auth::HELPER_AUTH_ERROR);
        return;
    }

    m_user = m_backend->userName();
    QProcessEnvironment env = authenticated(m_user);

    if (!m_session->path().isEmpty()) {
        env.insert(m_session->processEnvironment());
        m_session->setProcessEnvironment(env);

        if (!m_backend->openSession()) {
            sessionOpened(false);
            exit(Auth::HELPER_SESSION_ERROR);
            return;
        }

        sessionOpened(true);
    } else {
        exit(Auth::HELPER_SUCCESS);
    }
    return;
}

void HelperApp::sessionFinished(int status)
{
    exit(status);
}

void HelperApp::info(const QString &message, Auth::Info type)
{
    SafeDataStream str(m_socket);
    str << Msg::INFO << message << type;
    str.send();
    m_socket->waitForBytesWritten();
}

void HelperApp::error(const QString &message, Auth::Error type)
{
    SafeDataStream str(m_socket);
    str << Msg::ERROR << message << type;
    str.send();
    m_socket->waitForBytesWritten();
}

Request HelperApp::request(const Request &request)
{
    Msg m = Msg::MSG_UNKNOWN;
    Request response;
    SafeDataStream str(m_socket);
    str << Msg::REQUEST << request;
    str.send();
    str.receive();
    str >> m >> response;
    if (m != REQUEST) {
        response = Request();
        qCritical() << "Received a wrong opcode instead of REQUEST:" << m;
    }
    return response;
}

QProcessEnvironment HelperApp::authenticated(const QString &user)
{
    Msg m = Msg::MSG_UNKNOWN;
    QProcessEnvironment env;
    SafeDataStream str(m_socket);
    str << Msg::AUTHENTICATED << user;
    str.send();
    if (user.isEmpty()) {
        return env;
    }
    str.receive();
    str >> m >> env;
    if (m != AUTHENTICATED) {
        env = QProcessEnvironment();
        qCritical() << "Received a wrong opcode instead of AUTHENTICATED:" << m;
    }
    return env;
}

void HelperApp::sessionOpened(bool success)
{
    Msg m = Msg::MSG_UNKNOWN;
    SafeDataStream str(m_socket);
    str << Msg::SESSION_STATUS << success;
    str.send();
    str.receive();
    str >> m;
    if (m != SESSION_STATUS) {
        qCritical() << "Received a wrong opcode instead of SESSION_STATUS:" << m;
    }
}

void HelperApp::displayServerStarted(const QString &displayName)
{
    Msg m = Msg::MSG_UNKNOWN;
    SafeDataStream str(m_socket);
    str << Msg::DISPLAY_SERVER_STARTED << displayName;
    str.send();
    str.receive();
    str >> m;
    if (m != DISPLAY_SERVER_STARTED) {
        qCritical() << "Received a wrong opcode instead of DISPLAY_SERVER_STARTED:" << m;
    }
}

UserSession *HelperApp::session()
{
    return m_session;
}

const QString &HelperApp::user() const
{
    return m_user;
}

HelperApp::~HelperApp()
{
    Q_ASSERT(getuid() == 0);

    m_session->stop();
    m_backend->closeSession();
}
}

int main(int argc, char **argv)
{
    PLASMALOGIN::HelperApp app(argc, argv);
    return app.exec();
}

#include "moc_HelperApp.cpp"
