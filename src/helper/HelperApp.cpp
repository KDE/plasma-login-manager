/*
 * Main authentication application class
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "HelperApp.h"
#include "Configuration.h"
#include "MessageHandler.h"
#include "SafeDataStream.h"
#include "VirtualTerminal.h"
#include "backend/PamBackend.h"

#include <KSignalHandler>

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
    , m_socket(new QLocalSocket(this))
{
    qInstallMessageHandler(HelperMessageHandler);

    QTimer::singleShot(0, this, SLOT(setUp()));
}

void HelperApp::setUp()
{
    qDebug() << "set up";
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

    if ((pos = args.indexOf(QStringLiteral("--user"))) >= 0) {
        if (pos >= args.length() - 1) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }
        m_user = args[pos + 1];
    }

    if (m_id <= 0) {
        qCritical() << "This application is not supposed to be executed manually";
        exit(Auth::HELPER_OTHER_ERROR);
        return;
    }

    connect(m_socket, &QLocalSocket::connected, this, &HelperApp::doAuth);
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
        exit(Auth::HELPER_AUTH_ERROR);
        return;
    }

    Q_ASSERT(getuid() == 0);
    if (m_backend->authenticate()) {
        str << Msg::AUTHENTICATED << m_user;
        str.send();
        if (str.status() != QDataStream::Ok) {
            qCritical() << "Couldn't write initial message:" << str.status();
        }

        exit(Auth::HELPER_SUCCESS);
        return;
    }
    exit(Auth::HELPER_SUCCESS);
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

HelperApp::~HelperApp()
{
}
}

int main(int argc, char **argv)
{
    PLASMALOGIN::HelperApp app(argc, argv);
    return app.exec();
}

#include "moc_HelperApp.cpp"
