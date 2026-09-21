/*
 * SPDX-FileCopyrightText: 2025 Oliver Beard
 * SPDX-FileCopyrightText: 2025 2026 David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QQuickItem>

class Wallpaper : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit Wallpaper(QQuickItem *parent = nullptr);
    void componentComplete() override;
};
