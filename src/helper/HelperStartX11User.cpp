/*
 * Session process wrapper
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

/**
 * This application sole purpose is to launch an X11 rootless compositor compositor (first
 * argument) and as soon as it's set up to launch a client (second argument)
 */

#include <unistd.h>
#include <QCoreApplication>
#include <QTextStream>
#include <QProcess>
#include <QDebug>
#include "xorguserhelper.h"
#include "MessageHandler.h"
#include <signal.h>
#include "SignalHandler.h"

void X11UserHelperMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    PLASMALOGIN::messageHandler(type, context, QStringLiteral("X11UserHelper: "), msg);
}

int main(int argc, char** argv)
{
    qInstallMessageHandler(X11UserHelperMessageHandler);
    QCoreApplication app(argc, argv);
    PLASMALOGIN::SignalHandler s;
    QObject::connect(&s, &PLASMALOGIN::SignalHandler::sigtermReceived, &app, [] {
        QCoreApplication::instance()->exit(-1);
    });

    Q_ASSERT(::getuid() != 0);
    if (argc != 3) {
        QTextStream(stderr) << "Wrong number of arguments\n";
        return 33;
    }

    using namespace PLASMALOGIN;
    XOrgUserHelper helper;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &helper, [&helper] {
        qDebug("quitting helper-start-x11");
        helper.stop();
    });
    QObject::connect(&helper, &XOrgUserHelper::displayChanged, &app, [&helper, &app] {
        qDebug() << "starting XOrg Greeter..." << helper.sessionEnvironment().value(QStringLiteral("DISPLAY"));
        auto args = QProcess::splitCommand(app.arguments()[2]);

        QProcess *process = new QProcess(&app);
        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->setProgram(args.takeFirst());
        process->setArguments(args);
        process->setProcessEnvironment(helper.sessionEnvironment());
        process->start();
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &app, &QCoreApplication::quit);
    });

    helper.start(app.arguments()[1]);
    return app.exec();
}
