/*
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileSelector>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QUrl>

#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <Plasma/Plasma>
#include <kworkspace6/sessionmanagement.h>
#include <qqmlfileselector.h>

#include "backend/GreeterProxy.h"
#include "mockbackend/MockGreeterProxy.h"

#include "blurscreenbridge.h"
#include "greetereventfilter.h"
#include "models/sessionmodel.h"
#include "models/usermodel.h"
#include "plasmaloginsettings.h"
#include "stateconfig.h"

int main(int argc, char *argv[])
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("plasma-login"));

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("test"), QStringLiteral("Run in test mode")));
    parser.addHelpOption();

    QGuiApplication app(argc, argv);

    app.setQuitOnLastWindowClosed(false);

    parser.process(app);
    const bool testMode = parser.isSet(QStringLiteral("test"));

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QQuickWindow::setDefaultAlphaBuffer(true);
    if (testMode) {
        qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "Authenticator", new MockGreeterProxy);
    } else {
        qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "Authenticator", new PLASMALOGIN::GreeterProxy);
    }
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionModel", new SessionModel);
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "UserModel", new UserModel);
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionManagement", new SessionManagement());
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "Settings", &PlasmaLoginSettings::getInstance());
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "StateConfig", StateConfig::self());
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "BlurScreenBridge", new BlurScreenBridge);
    qmlRegisterType<GreeterEventFilter>("org.kde.plasma.login", 0, 1, "GreeterEventFilter");

    QQmlApplicationEngine engine;
    Plasma::setupPlasmaStyle(&engine);
    KLocalization::setupLocalizedContext(&engine);
    QQmlFileSelector selector(&engine);
    if (testMode) {
        selector.setExtraSelectors({QStringLiteral("test")});
    }
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/org/kde/plasma/login/Main.qml")));

    return app.exec();
}
