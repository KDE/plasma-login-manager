/*
 * SPDX-FileCopyrightText: 2025 Oliver Beard
 * SPDX-FileCopyrightText: 2025 2026 David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QUrl>
#include <qqmlapplicationengine.h>

#include <KLocalizedQmlContext>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma-login-wallpaper"));
    app.setQuitOnLastWindowClosed(false);

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/org/kde/plasma/login/wallpaper/main.qml")));

    return app.exec();
}
