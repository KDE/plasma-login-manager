/*
 * Session process wrapper
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

/**
 * This application sole purpose is to launch a wayland compositor (first
 * argument) and as soon as it's set up to launch a client (second argument)
 */

#include "Auth.h"
#include "MessageHandler.h"
#include "SignalHandler.h"
#include "waylandhelper.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include <signal.h>
#include <unistd.h>

void WaylandHelperMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    PLASMALOGIN::messageHandler(type, context, QStringLiteral("WaylandHelper: "), msg);
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(WaylandHelperMessageHandler);
    QCoreApplication app(argc, argv);
    using namespace PLASMALOGIN;
    PLASMALOGIN::SignalHandler s;

    Q_ASSERT(::getuid() != 0);
    if (argc != 3) {
        QTextStream(stderr) << "Wrong number of arguments\n";
        return Auth::HELPER_OTHER_ERROR;
    }

    WaylandHelper helper;
    QObject::connect(&s, &PLASMALOGIN::SignalHandler::sigtermReceived, &app, [] {
        QCoreApplication::exit(0);
    });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &helper, [&helper] {
        qDebug("quitting helper-start-wayland");
        helper.stop();
    });
    QObject::connect(&helper, &WaylandHelper::failed, &app, [&app] {
        QTextStream(stderr) << "Failed to start wayland session" << Qt::endl;
        app.exit(Auth::HELPER_SESSION_ERROR);
    });

    if (!helper.startCompositor(app.arguments()[1])) {
        qWarning() << "PLASMALOGIN was unable to start" << app.arguments()[1];
        return Auth::HELPER_DISPLAYSERVER_ERROR;
    }
    helper.startGreeter(app.arguments()[2]);
    return app.exec();
}
