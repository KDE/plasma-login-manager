/*
 *  SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *  SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCollator>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include "config.h"

#include "plasmaloginsettings.h"

PlasmaLoginSettings &PlasmaLoginSettings::getInstance()
{
    auto config = KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_CONFIG_FILE), KConfig::CascadeConfig);

    static PlasmaLoginSettings instance(config);
    return instance;
}

PlasmaLoginSettings::PlasmaLoginSettings(KSharedConfig::Ptr config, QObject *parent)
    : PlasmaLoginSettingsBase(config)
{
    setParent(parent);

    const auto wallpaperPackages = KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma/Wallpaper"));
    for (auto &package : wallpaperPackages) {
        m_availableWallpaperPlugins.append({package.name(), package.pluginId()});
    }
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_availableWallpaperPlugins.begin(), m_availableWallpaperPlugins.end(), [](const WallpaperInfo &w1, const WallpaperInfo &w2) {
        return w1.name < w2.name;
    });
}

PlasmaLoginSettings::~PlasmaLoginSettings()
{
}

QList<WallpaperInfo> PlasmaLoginSettings::availableWallpaperPlugins() const
{
    return m_availableWallpaperPlugins;
}

#include "moc_plasmaloginsettings.cpp"
