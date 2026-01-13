/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QQuickWindow>
#include <QSurfaceFormat>

#include "wallpaperapp.h"

int main(int argc, char **argv)
{
    QCoreApplication::setApplicationName(QStringLiteral("plasma-login-wallpaper"));

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication::setDesktopSettingsAware(false);
    WallpaperApp app(argc, argv);

    return app.exec();
}
