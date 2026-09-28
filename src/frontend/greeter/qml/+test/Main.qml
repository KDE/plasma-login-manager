/*
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Window

Window {
    id: window
    visible: true
    color: "grey"
    width: 1024
    height: 768
    Greeter {
        anchors.fill: parent
    }
}
