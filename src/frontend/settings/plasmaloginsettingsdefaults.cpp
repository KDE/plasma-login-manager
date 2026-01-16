/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "config.h"

#include "plasmaloginsettingsdefaults.h"

QString PlasmaLoginSettingsDefaults::s_defaultUser;
QString PlasmaLoginSettingsDefaults::s_defaultSession;
bool PlasmaLoginSettingsDefaults::s_defaultRelogin;
QString PlasmaLoginSettingsDefaults::s_defaultPreselectedUser;
QString PlasmaLoginSettingsDefaults::s_defaultPreselectedSession;
int PlasmaLoginSettingsDefaults::s_defaultShowClock;
QString PlasmaLoginSettingsDefaults::s_defaultWallpaperPluginId;

PlasmaLoginSettingsDefaults::PlasmaLoginSettingsDefaults(KSharedConfigPtr config, QObject *parent)
    : KConfigSkeleton(config, parent)
{
    auto defaultConfig = KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig);
    s_defaultUser = defaultConfig->group(QStringLiteral("AutoLogin")).readEntry("User", "");
    s_defaultSession = defaultConfig->group(QStringLiteral("AutoLogin")).readEntry("Session", "");
    s_defaultRelogin = defaultConfig->group(QStringLiteral("AutoLogin")).readEntry("Relogin", false);
    s_defaultPreselectedUser = defaultConfig->group(QStringLiteral("Greeter")).readEntry("PreselectedUser", "");
    s_defaultPreselectedSession = defaultConfig->group(QStringLiteral("Greeter")).readEntry("PreselectedSession", "");
    s_defaultShowClock = defaultConfig->group(QStringLiteral("Greeter")).readEntry("ShowClock", 0);
    s_defaultWallpaperPluginId = defaultConfig->group(QStringLiteral("Greeter")).readEntry("WallpaperPluginId", "org.kde.image");
}

QString PlasmaLoginSettingsDefaults::defaultUser()
{
    return s_defaultUser;
}

QString PlasmaLoginSettingsDefaults::defaultSession()
{
    return s_defaultSession;
}

bool PlasmaLoginSettingsDefaults::defaultRelogin()
{
    return s_defaultRelogin;
}

QString PlasmaLoginSettingsDefaults::defaultPreselectedUser()
{
    return s_defaultPreselectedUser;
}

QString PlasmaLoginSettingsDefaults::defaultPreselectedSession()
{
    return s_defaultPreselectedSession;
}

int PlasmaLoginSettingsDefaults::defaultShowClock()
{
    return s_defaultShowClock;
}

QString PlasmaLoginSettingsDefaults::defaultWallpaperPluginId()
{
    return s_defaultWallpaperPluginId;
}

#include "moc_plasmaloginsettingsdefaults.cpp"
