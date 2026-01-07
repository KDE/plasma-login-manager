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
    Q_OBJECT
    Q_PROPERTY(bool blur READ blur NOTIFY blurChanged)
public:
    WallpaperWindow(QScreen *screen);
    bool blur() const;
    void setBlur(bool enable);
Q_SIGNALS:
    void blurChanged();

private:
    QScreen *m_screen;
    bool m_blur = false;
};
