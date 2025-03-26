/*
SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>
SPDX-FileCopyrightText: 2024 Jakob Petsovits <jpetso@petsovits.com>
SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as ItemModels

import org.kde.private.kcms.plasmalogin

KCM.SimpleKCM {
    id: root

    implicitHeight: Kirigami.Units.gridUnit * 45
    implicitWidth: Kirigami.Units.gridUnit * 45

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Apply Plasma Settings…")
            icon.name: "plasma"
            onTriggered: syncSheet.open()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Configure Appearance…")
            icon.name: "edit-image-symbolic"
            onTriggered: kcm.push("Appearance.qml")
        }
    ]

    header: Kirigami.InlineMessage {
        id: errorMessage
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Error
        showCloseButton: true
        Connections {
            target: kcm

            function onErrorOccurred(untranslatedMessage) {
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
                onTriggered: kcm.resetSynchronizedSettings()
            }
        ]
    }

    ColumnLayout {
        spacing: 0

        Kirigami.FormLayout {

            Item {
                Kirigami.FormData.label: i18nc("@title:group", "Auto-login")
                Kirigami.FormData.isSection: true
            }

            RowLayout {
                Kirigami.FormData.label: i18nc("@option:check", "Automatically log in:")
                spacing: Kirigami.Units.smallSpacing

                QQC2.CheckBox {
                    id: autologinBox
                    text: i18nc("@label:listbox, the following combobox selects the user to log in automatically", "as user:")
                    checked: kcm.settings.user != ""
                    KCM.SettingHighlighter {
                        highlight: (kcm.settings.user != "" && kcm.settings.defaultUser == "") ||
                                    (kcm.settings.user == "" && kcm.settings.defaultUser != "")
                    }
                    onToggled: {
                        if (checked) {
                            kcm.settings.user = autologinUser.currentText
                            kcm.settings.session = autologinSession.currentValue
                        } else {
                            kcm.settings.user = ""
                            kcm.settings.session = ""
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
                    implicitWidth: Kirigami.Units.gridUnit * 12
                    model: UserModel {}
                    textRole: "display"
                    editable: true
                    onActivated: kcm.settings.user = currentText
                    KCM.SettingStateBinding {
                        visible: autologinBox.checked
                        configObject: kcm.settings
                        settingName: "User"
                        extraEnabledConditions: autologinBox.checked
                    }
                    Component.onCompleted: {
                        updateSelectedUser();

                        // In the initial state, comboBox sets currentIndex to 0 (the first value from the comboBox).
                        // After component is completed currentIndex changes to the correct value using `updateSelectUser` here.
                        // This implicit initial changing of the currentIndex (to 0) calls the onEditTextChanged handler,
                        // which in turn saves the wrong login in kcm.settings.user (the first value from the comboBox).
                        // So we need connect to editTextChanged signal here after the correct currentIndex was settled
                        // thus reacting only to user input and pressing the Reset/Default buttons.
                        autologinUser.editTextChanged.connect(setUserFromEditText);
                    }
                    function setUserFromEditText() {
                        kcm.settings.user = editText;
                    }
                    function updateSelectedUser() {
                        const index = find(kcm.settings.user);
                        if (index != -1) {
                            currentIndex = index;
                        }
                        editText = kcm.settings.user;
                    }
                    Connections { // Needed for "Reset" and "Default" buttons to work
                        target: kcm.settings
                        function onUserChanged() { autologinUser.updateSelectedUser(); }
                    }
                }
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    Layout.leftMargin: autologinBox.contentItem.leftPadding
                    text: i18nc("@label:listbox, the following combobox selects the session that is started automatically", "with session:")
                }
                QQC2.ComboBox {
                    id: autologinSession
                    implicitWidth: Kirigami.Units.gridUnit * 12
                    model: SessionModel {}
                    textRole: "display"
                    valueRole: "path"
                    onActivated: kcm.settings.session = currentValue
                    KCM.SettingStateBinding {
                        visible: autologinBox.checked
                        configObject: kcm.settings
                        settingName: "Session"
                        extraEnabledConditions: autologinBox.checked
                    }
                    Component.onCompleted: updateCurrentIndex()
                    function updateCurrentIndex() {
                        currentIndex = Math.max(indexOfValue(kcm.settings.session), 0);
                    }
                    Connections { // Needed for "Reset" and "Default" buttons to work
                        target: kcm.settings
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
                checked: kcm.settings.relogin
                onToggled: kcm.settings.relogin = checked
                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "Relogin"
                    extraEnabledConditions: autologinBox.checked
                }
            }

            Item {
                Kirigami.FormData.label: i18nc("@title:group", "Defaults")
                Kirigami.FormData.isSection: true
            }

            QQC2.RadioButton {
                Kirigami.FormData.label: i18nc("@label", "Default user:")

                text: i18nc("@option:radio", "Last logged-in user")
                checked: kcm.settings.preselectedUser == ""
                onToggled: {
                    if (checked) {
                        kcm.settings.preselectedUser = "";
                    }
                }

                KCM.SettingHighlighter {
                    highlight: kcm.settings.preselectedUser != kcm.settings.defaultPreselectedUser
                }
            }

            RowLayout {
                spacing: 0

                QQC2.RadioButton {
                    id: customPreselectedUserRadioButton

                    checked: kcm.settings.preselectedUser != ""
                    onToggled: {
                        if (checked) {
                            kcm.settings.preselectedUser = customPreselectedUserComboBox.currentText
                        }
                    }

                    KCM.SettingHighlighter {
                        highlight: kcm.settings.preselectedUser != kcm.settings.defaultPreselectedUser
                    }
                }

                QQC2.ComboBox {
                    id: customPreselectedUserComboBox

                    implicitWidth: Kirigami.Units.gridUnit * 12
                    model: UserModel {}
                    textRole: "display"
                    editable: true
                    onActivated: kcm.settings.preselectedUser = currentText
                    onEditTextChanged: kcm.settings.preselectedUser = editText;

                    Component.onCompleted: updateSelectedUser()
                    Connections {
                        target: kcm.settings
                        function onPreselectedUserChanged() { customPreselectedUserComboBox.updateSelectedUser(); }
                    }

                    function updateSelectedUser() {
                        const index = find(kcm.settings.preselectedUser);
                        if (index != -1) {
                            currentIndex = index;
                        }
                        editText = kcm.settings.preselectedUser;
                    }

                    KCM.SettingStateBinding {
                        visible: customPreselectedUserRadioButton.checked
                        configObject: kcm.settings
                        settingName: "PreselectedUser"
                        extraEnabledConditions: customPreselectedUserRadioButton.checked
                    }
                }
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.RadioButton {
                Kirigami.FormData.label: i18nc("@label", "Default session:")

                text: i18nc("@option:radio", "Last logged-in session")
                checked: kcm.settings.preselectedSession == ""
                onToggled: {
                    if (checked) {
                        kcm.settings.preselectedSession = "";
                    }
                }

                KCM.SettingHighlighter {
                    highlight: kcm.settings.preselectedSession != kcm.settings.defaultPreselectedSession
                }
            }

            RowLayout {
                spacing: 0

                QQC2.RadioButton {
                    id: customPreselectedSessionRadioButton

                    checked: kcm.settings.preselectedSession != ""
                    onToggled: {
                        if (checked) {
                            kcm.settings.preselectedSession = customPreselectedSessionComboBox.valueAt(0);
                        }
                    }

                    KCM.SettingHighlighter {
                        highlight: kcm.settings.preselectedSession != kcm.settings.defaultPreselectedSession
                    }
                }

                QQC2.ComboBox {
                    id: customPreselectedSessionComboBox

                    implicitWidth: Kirigami.Units.gridUnit * 12
                    model: SessionModel {}
                    textRole: "display"
                    valueRole: "path"
                    onActivated: kcm.settings.preselectedSession = currentValue

                    Component.onCompleted: updateCurrentIndex()
                    Connections {
                        target: kcm.settings
                        function onPreselectedSessionChanged() { customPreselectedSessionComboBox.updateCurrentIndex(); }
                    }

                    function updateCurrentIndex() {
                        currentIndex = indexOfValue(kcm.settings.preselectedSession);
                    }

                    KCM.SettingStateBinding {
                        visible: customPreselectedSessionRadioButton.checked
                        configObject: kcm.settings
                        settingName: "PreselectedSession"
                        extraEnabledConditions: customPreselectedSessionRadioButton.checked
                    }
                }
            }
        }
    }
}
