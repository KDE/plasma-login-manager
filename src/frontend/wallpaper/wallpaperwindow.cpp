/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wallpaperwindow.h"

WallpaperWindow::WallpaperWindow()
    : PlasmaQuick::QuickViewSharedEngine()
{
}

bool WallpaperWindow::blur() const
{
    return m_blur;
}

void WallpaperWindow::setBlur(bool enable)
{
    if (m_blur == enable) {
        return;
    }

    m_blur = enable;
    Q_EMIT blurChanged();
}
