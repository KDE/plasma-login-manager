/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "config.h"

#include "plasmaloginsettingsdefaults.h"

PlasmaLoginSettingsDefaults::PlasmaLoginSettingsDefaults(KSharedConfigPtr config, QObject *parent)
    : KConfigSkeleton(config, parent)
{
}

unsigned int PlasmaLoginSettingsDefaults::defaultMinimumUid()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("Users")).readEntry("MinimumUid", 1000);
}

unsigned int PlasmaLoginSettingsDefaults::defaultMaximumUid()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("Users")).readEntry("MaximumUid", 60000);
}

QString PlasmaLoginSettingsDefaults::defaultUser()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("AutoLogin")).readEntry("User");
}

QString PlasmaLoginSettingsDefaults::defaultSession()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("AutoLogin")).readEntry("Session");
}

bool PlasmaLoginSettingsDefaults::defaultRelogin()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("AutoLogin")).readEntry("Relogin", false);
}

QString PlasmaLoginSettingsDefaults::defaultHaltCommand()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("General")).readEntry("HaltCommand");
}

QString PlasmaLoginSettingsDefaults::defaultRebootCommand()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("General")).readEntry("RebootCommand");
}

bool PlasmaLoginSettingsDefaults::defaultShowClock()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("Greeter")).readEntry("ShowClock", true);
}

QString PlasmaLoginSettingsDefaults::defaultWallpaperPluginId()
{
    return KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_FILE), KConfig::CascadeConfig)->group(QStringLiteral("Greeter")).readEntry("WallpaperPluginId", "org.kde.image");
}
