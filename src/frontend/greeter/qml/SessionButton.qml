/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15

import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.20 as Kirigami

import org.kde.plasma.login as PlasmaLogin

PlasmaComponents.ToolButton {
    id: root

    property int currentIndex: PlasmaLogin.GreeterState.sessionIndex

    readonly property int currentSessionType: instantiator.model.data(instantiator.model.index(currentIndex, 0), PlasmaLogin.SessionModel.TypeRole)
    readonly property string currentSessionFileName: instantiator.model.data(instantiator.model.index(currentIndex, 0), PlasmaLogin.SessionModel.FileNameRole)
    readonly property string currentSessionPath: instantiator.model.data(instantiator.model.index(currentIndex, 0), PlasmaLogin.SessionModel.PathRole)

    // Count is used as instantiator may not have made items yet
    text: i18nd("plasma_login", "Desktop Session: %1", instantiator.count > currentIndex ? instantiator.objectAt(currentIndex).text : "")
    visible: menu.count > 1

    checkable: true
    checked: menu.opened
    onToggled: {
        if (checked) {
            menu.popup(root, 0, 0)
        } else {
            menu.dismiss()
        }
    }

    signal sessionChanged()

    PlasmaComponents.Menu {
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false

        id: menu
        Instantiator {
            id: instantiator
            model: PlasmaLogin.SessionModel
            onObjectAdded: (index, object) => menu.insertItem(index, object)
            onObjectRemoved: (index, object) => menu.removeItem(object)
            delegate: PlasmaComponents.MenuItem {
                PlasmaComponents.ToolTip.text: model.comment
                PlasmaComponents.ToolTip.visible: hovered
                PlasmaComponents.ToolTip.delay: Kirigami.Units.toolTipDelay

                text: model.display
                onTriggered: {
                    root.currentIndex = model.index
                    sessionChanged()
                }
            }
        }
    }

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
