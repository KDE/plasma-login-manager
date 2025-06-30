/*
 *  SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "plasmaloginsettings.h"
#include "wallpapersettings.h"

#include "plasmalogindata.h"

PlasmaLoginData::PlasmaLoginData(QObject *parent)
    : KCModuleData(parent)
    , m_wallpaperSettings(new WallpaperSettings(this))
{
    m_wallpaperSettings->load();
}

bool PlasmaLoginData::isDefaults() const
{
    return PlasmaLoginSettings::getInstance().isDefaults() && m_wallpaperSettings->isDefaults();
}

#include "moc_plasmalogindata.cpp"
