/*
 * SPDX-FileCopyrightText: Oliver Beard
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window

import org.kde.kirigami as Kirigami

import org.kde.plasma.login.wallpaper as PlasmaLoginWallpaper

Item {
    id: main
    anchors.fill: parent

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    property alias wallpaperContainer: wallpaperPlaceholder

    Item {
        id: wallpaperPlaceholder
        anchors.fill: parent
    }

    PlasmaLoginWallpaper.WallpaperFader {
        anchors.fill: parent
        factor: Window.window?.blur ? 1 : 0
        source: wallpaperPlaceholder.children[0]
    }
}
