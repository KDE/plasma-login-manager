/*
 * SPDX-FileCopyrightText: Oliver Beard
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window

import org.kde.kirigami as Kirigami
import org.kde.layershell 1.0 as LayerShell

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.login.wallpaper as PlasmaLoginWallpaper

Instantiator {
    model: PlasmaCore.ScreensModel {}

    delegate: Window {
        id: window
        visible: true
        color: "black"

        required property var screenHandle
        property bool blur: PlasmaLoginWallpaper.BlurAdaptor.activeScreen === screenHandle.name

        LayerShell.Window.layer: LayerShell.Window.LayerBackground;
        LayerShell.Window.exclusionZone: -1;
        LayerShell.Window.scope: "plasma-login-wallpaper"
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
        LayerShell.Window.screen: screenHandle

        PlasmaLoginWallpaper.Wallpaper {
            id: wallpaperPlaceholder
            anchors.fill: parent

            Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
            Kirigami.Theme.inherit: false
        }

        PlasmaLoginWallpaper.WallpaperFader {
            anchors.fill: parent
            factor: window?.blur ? 1 : 0
            source: wallpaperPlaceholder
        }
    }
}
