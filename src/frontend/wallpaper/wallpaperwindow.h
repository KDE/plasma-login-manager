/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QScreen>

#include <PlasmaQuick/QuickViewSharedEngine>

class WallpaperWindow : public PlasmaQuick::QuickViewSharedEngine
{
public:
    WallpaperWindow(QScreen *screen);

private:
    QScreen *m_screen;
};
