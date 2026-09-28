/*
 * SPDX-FileCopyrightText: Oliver Beard
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window

import org.kde.layershell 1.0 as LayerShell
import org.kde.plasma.core as PlasmaCore

Instantiator {
    model: PlasmaCore.ScreensModel {}

    delegate: Window {
        id: window
        visible: true
        color: "transparent"

        required property var screenHandle

        LayerShell.Window.layer: LayerShell.Window.LayerTop
        LayerShell.Window.exclusionZone: -1
        LayerShell.Window.scope: "plasma-login-greeter"
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityExclusive
        LayerShell.Window.screen: screenHandle
        Greeter {
            anchors.fill: parent
        }
    }
}
