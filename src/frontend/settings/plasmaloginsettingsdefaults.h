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

public:
    PlasmaLoginSettingsDefaults(KSharedConfigPtr config, QObject *parent = nullptr);

protected:
    static QString defaultUser();
    static QString defaultSession();
    static bool defaultRelogin();
    static bool defaultShowClock();
    static QString defaultWallpaperPluginId();
};
