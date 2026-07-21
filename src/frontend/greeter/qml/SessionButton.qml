/*
    SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

import org.kde.plasma.login as PlasmaLogin

PlasmaComponents.ComboBox {
    id: root

    model: PlasmaLogin.SessionModel
    textRole: "display"

    visible: count > 1
    flat: true
    displayText: i18nd("plasma_login", "Desktop Session: %1", currentText)

    contentItem: QQC2.Label {
        font: root.font
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        text: root.displayText
    }

    PlasmaComponents.ToolTip.text: currentText
    PlasmaComponents.ToolTip.visible: hovered && contentItem.truncated && !popup.visible
    PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay

    Binding {
        target: PlasmaLogin.GreeterState
        property: "greeterHasSessionButtonPopup"
        value: true
        when: popup.visible
    }

    currentIndex: PlasmaLogin.GreeterState.sessionIndex

    Connections {
        target: PlasmaLogin.GreeterState

        function onSessionIndexChanged() {
            if (root.currentIndex != PlasmaLogin.GreeterState.sessionIndex) {
                root.currentIndex = PlasmaLogin.GreeterState.sessionIndex;
            }
        }
    }

    onCurrentIndexChanged: {
        PlasmaLogin.GreeterState.sessionIndex = root.currentIndex;
    }
}
