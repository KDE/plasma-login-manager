/*
 *  SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include <KConfigPropertyMap>
#include <KCoreConfigSkeleton>
#include <KPackage/Package>

#include "wallpaperintegration.h"

class WallpaperSettings : public QObject
{
    Q_OBJECT

public:
    explicit WallpaperSettings(QObject *parent = nullptr);

    QUrl wallpaperConfigFile() const;

    KConfigPropertyMap *wallpaperConfiguration() const;

    WallpaperIntegration *wallpaperIntegration() const;

    void load();
    void save();
    void defaults();

    bool isDefaults() const;
    bool isSaveNeeded() const;

    void loadWallpaperConfig();

Q_SIGNALS:
    void currentWallpaperChanged();

private:
    // KPackage::Package m_package;
    WallpaperIntegration *m_wallpaperIntegration = nullptr;
    KCoreConfigSkeleton *m_wallpaperSettings = nullptr;
    QUrl m_wallpaperConfigFile;
};
