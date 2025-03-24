/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.workspace.components as WorkspaceComponents

PlasmaComponents.ToolButton {
    focusPolicy: Qt.TabFocus
    Accessible.description: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to change keyboard layout", "Switch layout")
    icon.name: "input-keyboard"

    WorkspaceComponents.KeyboardLayoutSwitcher {
        id: keyboardLayoutSwitcher

        anchors.fill: parent
        acceptedButtons: Qt.NoButton
    }

    text: keyboardLayoutSwitcher.layoutNames.longName
    onClicked: keyboardLayoutSwitcher.keyboardLayout.switchToNextLayout()

    visible: keyboardLayoutSwitcher.hasMultipleKeyboardLayouts
}
