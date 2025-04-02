/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QGuiApplication>
#include <QObject>

#include <KPackage/PackageStructure>

namespace PlasmaQuick
{
class QuickViewSharedEngine;
}

class WallpaperIntegration;

class WallpaperWindow;

class WallpaperApp : public QGuiApplication
{
    Q_OBJECT

public:
    explicit WallpaperApp(int &argc, char **argv);
    ~WallpaperApp() override;

private:
    void setupWallpaperPlugin(WallpaperWindow *window);

    KPackage::Package m_wallpaperPackage;
    QList<WallpaperWindow *> m_windows;

private Q_SLOTS:
    void adoptScreen(QScreen *);
};
