/*
 *  SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KConfigLoader>
#include <KPackage/PackageLoader>

#include "plasmaloginsettings.h"

#include "wallpapersettings.h"

WallpaperSettings::WallpaperSettings(QObject *parent)
    : QObject(parent)
{
}

QUrl WallpaperSettings::wallpaperConfigFile() const
{
    return m_wallpaperConfigFile;
}

KConfigPropertyMap *WallpaperSettings::wallpaperConfiguration() const
{
    if (!m_wallpaperIntegration) {
        return nullptr;
    }
    return m_wallpaperIntegration->configuration();
}

WallpaperIntegration *WallpaperSettings::wallpaperIntegration() const
{
    return m_wallpaperIntegration.get();
}

KCoreConfigSkeleton *WallpaperSettings::wallpaperSkeleton() const
{
    return m_wallpaperSettings;
}

void WallpaperSettings::load(PlasmaLoginSettings *plasmaLoginSettings)
{
    loadWallpaperConfig(plasmaLoginSettings);

    if (m_wallpaperSettings) {
        m_wallpaperSettings->load();
        Q_EMIT m_wallpaperSettings->configChanged(); // To force the ConfigPropertyMap to reevaluate
    }
}

void WallpaperSettings::save()
{
    if (m_wallpaperSettings) {
        m_wallpaperSettings->save();
    }
}

void WallpaperSettings::defaults()
{
    if (m_wallpaperSettings) {
        m_wallpaperSettings->setDefaults();
        Q_EMIT m_wallpaperSettings->configChanged(); // To force the ConfigPropertyMap to reevaluate
    }
}

bool WallpaperSettings::isDefaults() const
{
    bool defaults = true;

    if (m_wallpaperSettings) {
        defaults &= m_wallpaperSettings->isDefaults();
    }
    return defaults;
}

bool WallpaperSettings::isSaveNeeded() const
{
    bool saveNeeded = false;

    if (m_wallpaperSettings) {
        saveNeeded |= m_wallpaperSettings->isSaveNeeded();
    }

    return saveNeeded;
}

void WallpaperSettings::loadWallpaperConfig(PlasmaLoginSettings *plasmaLoginSettings)
{
    if (m_wallpaperIntegration) {
        if (m_wallpaperIntegration->pluginName() == plasmaLoginSettings->wallpaperPluginId()) {
            // nothing changed
            return;
        }
    }

    m_wallpaperIntegration = std::make_unique<WallpaperIntegration>();
    m_wallpaperIntegration->setConfig(plasmaLoginSettings->sharedConfig());
    m_wallpaperIntegration->setPluginName(plasmaLoginSettings->wallpaperPluginId());
    m_wallpaperIntegration->init();

    // TODO: Why can't we delete the old skeleton? (crash in KCM on wallpaper plugin change)
    /*
    if (m_wallpaperSettings) {
        m_wallpaperSettings->deleteLater();
    }
    */
    m_wallpaperSettings = m_wallpaperIntegration->configScheme();

    m_wallpaperConfigFile = m_wallpaperIntegration->package().fileUrl(QByteArrayLiteral("ui"), QStringLiteral("config.qml"));

    Q_EMIT currentWallpaperChanged();
}

#include "moc_wallpapersettings.cpp"
