/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KConfigSkeleton>

class PlasmaLoginSettingsDefaults : public KConfigSkeleton
{
    Q_OBJECT

    Q_PROPERTY(QString defaultUser READ defaultUser CONSTANT)
    Q_PROPERTY(QString defaultSession READ defaultSession CONSTANT)
    Q_PROPERTY(bool defaultRelogin READ defaultRelogin CONSTANT)
    Q_PROPERTY(QString defaultPreselectedUser READ defaultPreselectedUser CONSTANT)
    Q_PROPERTY(QString defaultPreselectedSession READ defaultPreselectedSession CONSTANT)
    Q_PROPERTY(bool defaultShowClock READ defaultShowClock CONSTANT)
    Q_PROPERTY(QString defaultWallpaperPluginId READ defaultWallpaperPluginId CONSTANT)

public:
    PlasmaLoginSettingsDefaults(KSharedConfigPtr config, QObject *parent = nullptr);

    static QString defaultUser();
    static QString defaultSession();
    static bool defaultRelogin();
    static QString defaultPreselectedUser();
    static QString defaultPreselectedSession();
    static bool defaultShowClock();
    static QString defaultWallpaperPluginId();

private:
    static QString s_defaultUser;
    static QString s_defaultSession;
    static bool s_defaultRelogin;
    static QString s_defaultPreselectedUser;
    static QString s_defaultPreselectedSession;
    static bool s_defaultShowClock;
    static QString s_defaultWallpaperPluginId;
};
