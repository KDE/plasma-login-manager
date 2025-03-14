/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as ItemModels

import org.kde.private.kcms.plasmalogin

KCM.SimpleKCM {
    id: root

    implicitWidth: Kirigami.Units.gridUnit * 36

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Apply Plasma Settings…")
            icon.name: "plasma"
            onTriggered: syncSheet.open()
        }
    ]

    headerPaddingEnabled: false // Let the InlineMessage touch the edges
    header: Kirigami.InlineMessage {
        id: errorMessage
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Error
        showCloseButton: true
        Connections {
            target: kcm

            function onErrorOccured(untranslatedMessage) {
                errorMessage.text = i18n(untranslatedMessage);
                errorMessage.visible = untranslatedMessage.length > 0
            }

            function onSyncAttempted() {
                syncSheet.close()
            }
        }
    }

    Kirigami.PromptDialog {
        id: syncSheet

        padding: Kirigami.Units.largeSpacing
        standardButtons: Kirigami.Dialog.Cancel

        title: i18nc("@title:window", "Apply Plasma Settings")
        subtitle: i18n("This will make the Plasma login screen reflect your customizations to the following Plasma settings:") +
                xi18nc("@info", "<para><list><item>Color scheme</item><item>Cursor theme and size</item><item>Font and font rendering</item><item>NumLock preference</item><item>Plasma theme</item><item>Scaling DPI</item><item>Screen configuration (Wayland only)</item></list></para>") +
                i18n("Please note that theme files must be installed globally to be reflected on the Plasma login screen.")

        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Apply")
                icon.name: "dialog-ok-apply"
                onTriggered: kcm.synchronizeSettings()
            },
            Kirigami.Action {
                text: i18nc("@action:button", "Reset to Default Settings")
                icon.name: "edit-undo"
                onTriggered: kcm.resetSyncronizedSettings()
            }
        ]
    }

    Kirigami.FormLayout {
        RowLayout {
            Kirigami.FormData.label: i18nc("option:check", "Automatically log in:")
            spacing: Kirigami.Units.smallSpacing

            QQC2.CheckBox {
                id: autologinBox
                text: i18nc("@label:listbox, the following combobox selects the user to log in automatically", "as user:")
                checked: kcm.plasmaLoginSettings.user != ""
                KCM.SettingHighlighter {
                    highlight: (kcm.plasmaLoginSettings.user != "" && kcm.plasmaLoginSettings.defaultUser == "") ||
                                (kcm.plasmaLoginSettings.user == "" && kcm.plasmaLoginSettings.defaultUser != "")
                }
                onToggled: {

                    if (checked) {
                        kcm.plasmaLoginSettings.user = autologinUser.currentText
                        kcm.plasmaLoginSettings.session = autologinSession.currentValue
                    } else {
                        kcm.plasmaLoginSettings.user = ""
                        kcm.plasmaLoginSettings.session = ""
                    }

                    // Deliberately imperative because we only want the message
                    // to appear when the user checks the checkbox, not all the
                    // time when the checkbox is checked.
                    if (checked && kcm.KDEWalletAvailable()) {
                        autologinMessage.visible = true;
                    }
                }
            }
            QQC2.ComboBox {
                id: autologinUser
                model: ItemModels.KSortFilterProxyModel {
                    sourceModel: UsersModel {
                        id: userModel
                    }
                    filterRowCallback: function(sourceRow, sourceParent) {
                        const id = userModel.data(userModel.index(sourceRow, 0, sourceParent), UsersModel.UidRole)
                        return kcm.plasmaLoginSettings.minimumUid <= id && id <= kcm.plasmaLoginSettings.maximumUid
                    }
                }
                textRole: "display"
                editable: true
                onActivated: kcm.plasmaLoginSettings.user = currentText
                KCM.SettingStateBinding {
                    visible: autologinBox.checked
                    configObject: kcm.plasmaLoginSettings
                    settingName: "User"
                    extraEnabledConditions: autologinBox.checked
                }
                Component.onCompleted: {
                    updateSelectedUser();

                    // In the initial state, comboBox sets currentIndex to 0 (the first value from the comboBox).
                    // After component is completed currentIndex changes to the correct value using `updateSelectUser` here.
                    // This implicit initial changing of the currentIndex (to 0) calls the onEditTextChanged handler,
                    // which in turn saves the wrong login in kcm.plasmaLoginSettings.user (the first value from the comboBox).
                    // So we need connect to editTextChanged signal here after the correct currentIndex was settled
                    // thus reacting only to user input and pressing the Reset/Default buttons.
                    autologinUser.editTextChanged.connect(setUserFromEditText);
                }
                function setUserFromEditText() {
                    kcm.plasmaLoginSettings.user = editText;
                }
                function updateSelectedUser() {
                    const index = find(kcm.plasmaLoginSettings.user);
                    if (index != -1) {
                        currentIndex = index;
                    }
                    editText = kcm.plasmaLoginSettings.user;
                }
                Connections { // Needed for "Reset" and "Default" buttons to work
                    target: kcm.plasmaLoginSettings
                    function onUserChanged() { autologinUser.updateSelectedUser(); }
                }
            }
            QQC2.Label {
                enabled: autologinBox.checked
                text: i18nc("@label:listbox, the following combobox selects the session that is started automatically", "with session")
            }
            QQC2.ComboBox {
                id: autologinSession
                model: SessionModel {}
                textRole: "name"
                valueRole: "file"
                onActivated: kcm.plasmaLoginSettings.session = currentValue
                KCM.SettingStateBinding {
                    visible: autologinBox.checked
                    configObject: kcm.plasmaLoginSettings
                    settingName: "Session"
                    extraEnabledConditions: autologinBox.checked
                }
                Component.onCompleted: updateCurrentIndex()
                function updateCurrentIndex() {
                    currentIndex = Math.max(indexOfValue(kcm.plasmaLoginSettings.session), 0);
                }
                Connections { // Needed for "Reset" and "Default" buttons to work
                    target: kcm.plasmaLoginSettings
                    function onSessionChanged() { autologinSession.updateCurrentIndex(); }
                }
            }
        }
        Kirigami.InlineMessage {
            id: autologinMessage

            Layout.fillWidth: true

            type: Kirigami.MessageType.Warning

            text: xi18nc("@info", "Auto-login does not support unlocking your KDE Wallet automatically, so it will ask you to unlock it every time you log in.\
<nl/><nl/>\
To avoid this, you can change the wallet to have a blank password. Note that this is insecure and should only be done in a trusted environment.")

            actions: Kirigami.Action {
                text: i18n("Open KDE Wallet Settings")
                icon.name: "kwalletmanager"
                onTriggered: kcm.openKDEWallet();
            }
        }
        QQC2.CheckBox {
            text: i18nc("@option:check", "Log in again immediately after logging off")
            checked: kcm.plasmaLoginSettings.relogin
            onToggled: kcm.plasmaLoginSettings.relogin = checked
            KCM.SettingStateBinding {
                configObject: kcm.plasmaLoginSettings
                settingName: "Relogin"
                extraEnabledConditions: autologinBox.checked
            }
        }
        Item {
            Kirigami.FormData.isSection: true
        }
        QQC2.SpinBox {
            Kirigami.FormData.label: i18nc("@label:spinbox", "Minimum user UID:")
            id: minSpinBox
            from: 1000
            to: maxSpinBox.value
            value: kcm.plasmaLoginSettings.minimumUid
            onValueModified: kcm.plasmaLoginSettings.minimumUid = value
            KCM.SettingStateBinding {
                configObject: kcm.plasmaLoginSettings
                settingName: "MinimumUid"
            }
        }
        QQC2.SpinBox {
            Kirigami.FormData.label: i18nc("@label:spinbox", "Maximum user UID:")
            id: maxSpinBox
            from: minSpinBox.value
            to: 60513
            value: kcm.plasmaLoginSettings.maximumUid
            onValueModified: kcm.plasmaLoginSettings.maximumUid = value
            KCM.SettingStateBinding {
                configObject: kcm.plasmaLoginSettings
                settingName: "MaximumUid"
            }
        }
        Item {
            Kirigami.FormData.isSection: true
        }
        RowLayout {
            Kirigami.FormData.label: i18nc("@label:textbox", "Halt Command:")
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.ActionTextField {
                id: haltField
                Layout.fillWidth: true
                text: kcm.plasmaLoginSettings.haltCommand
                readOnly: false
                onTextChanged: kcm.plasmaLoginSettings.haltCommand = text
                rightActions: [ Kirigami.Action {
                    icon.name: haltField.LayoutMirroring.enabled ? "edit-clear-locationbar-ltr" : "edit-clear-locationbar-rtl"
                    visible: haltField.text.length > 0
                    onTriggered: kcm.plasmaLoginSettings.haltCommand = ""
                }]
                KCM.SettingStateBinding {
                    configObject: kcm.plasmaLoginSettings
                    settingName: "HaltCommand"
                }
            }
            QQC2.Button {
                id: haltButton
                icon.name: "document-open-folder"
                enabled: haltField.enabled
                function selectFile() {
                    fileDialog.handler = (url => kcm.plasmaLoginSettings.haltCommand = kcm.toLocalFile(url))
                    fileDialog.open()
                }
                onClicked: selectFile()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Kirigami.FormData.label: i18nc("@label:textbox", "Reboot Command:")
            spacing: Kirigami.Units.smallSpacing

            Kirigami.ActionTextField {
                id: rebootField
                Layout.fillWidth: true
                text: kcm.plasmaLoginSettings.rebootCommand
                readOnly: false
                onTextChanged: kcm.plasmaLoginSettings.rebootCommand = text
                rightActions: [ Kirigami.Action {
                    icon.name: rebootField.LayoutMirroring.enabled ? "edit-clear-locationbar-ltr" : "edit-clear-locationbar-rtl"
                    visible: rebootField.text.length > 0
                    onTriggered: kcm.plasmaLoginSettings.rebootCommand = ""
                }]
                KCM.SettingStateBinding {
                    configObject: kcm.plasmaLoginSettings
                    settingName: "RebootCommand"
                }
            }
            QQC2.Button {
                id: rebootButton
                icon.name: "document-open-folder"
                enabled: rebootField.enabled
                function selectFile() {
                    fileDialog.handler = (url => kcm.plasmaLoginSettings.rebootCommand = kcm.toLocalFile(url))
                    fileDialog.open()
                }
                onClicked: selectFile()
            }
        }
        FileDialog {
            id: fileDialog
            property var handler
            onAccepted: {
                handler(fileUrl)
            }
        }
    }
}
