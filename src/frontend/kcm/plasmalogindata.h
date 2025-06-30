/*
 *  SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include "kcmoduledata.h"
#include "wallpapersettings.h"

class PlasmaLoginData : public KCModuleData
{
    Q_OBJECT

public:
    explicit PlasmaLoginData(QObject *parent);

    bool isDefaults() const override;

private:
    WallpaperSettings *m_wallpaperSettings = nullptr;
};
