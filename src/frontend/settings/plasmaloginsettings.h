/*
 *  SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "plasmaloginsettingsbase.h"

struct WallpaperInfo {
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QString id MEMBER id CONSTANT)
    QString name;
    QString id;
    Q_GADGET
};

class PlasmaLoginSettings : public PlasmaLoginSettingsBase
{
    Q_OBJECT

public:
    static PlasmaLoginSettings &getInstance();

    ~PlasmaLoginSettings() override;

    QList<WallpaperInfo> availableWallpaperPlugins() const;

    PlasmaLoginSettings(PlasmaLoginSettings const &) = delete;
    void operator=(PlasmaLoginSettings const &) = delete;

protected:
    PlasmaLoginSettings(KSharedConfig::Ptr config, QObject *parent = nullptr);

private:
    QList<WallpaperInfo> m_availableWallpaperPlugins;
};
