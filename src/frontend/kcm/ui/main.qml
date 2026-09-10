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
                xi18nc("@info", "<para><list><item>Language and locale settings</item><item>Keyboard layouts</item><item>Screen arrangement, scale, and orientation settings</item><item>NumLock preference</item><item>Font and font rendering settings</item><item>Color scheme</item><item>Cursor theme and size</item><item>Plasma theme</item></list></para>") +
                i18n("Please note that theme and font files must be installed globally to be reflected on the Plasma login screen.")

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

    Kirigami.Form {

        Kirigami.FormGroup {
            title: i18nc("@title:group", "Auto-login")

            Kirigami.FormEntry {
                title: i18nc("part of a sentence: 'Automatically log in [as user: foo / with session: bar]'", "Automatically log in:")

                contentItem: ColumnLayout {
                    Kirigami.FormData.buddyFor: autologinBox

                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.CheckBox {
                            id: autologinBox
                            Layout.preferredWidth: Math.max(width, autoLoginSessionLabel.width)

                            text: i18nc("@label:listbox part of a sentence: '[Automatically log in] as user: [foo / with session: bar]'", "as user:")
                            checked: kcm.settings.user != ""
                            onToggled: {
                                if (checked) {
                                    kcm.settings.user = autologinUser.indexOfValue(kcm.currentUser) !== -1 ? kcm.currentUser : autologinUser.valueAt(0);
                                    kcm.settings.session = autologinSession.valueAt(0);
                                } else {
                                    kcm.settings.user = "";
                                    kcm.settings.session = "";
                                }

                                // Deliberately imperative because we only want the message
                                // to appear when the user checks the checkbox, not all the
                                // time when the checkbox is checked.
                                if (checked && kcm.KDEWalletAvailable()) {
                                    autologinMessage.visible = true;
                                }
                            }

                            KCM.SettingHighlighter {
                                highlight: (kcm.settings.user != "" && kcm.settings.defaultUser == "")
                                            || (kcm.settings.user == "" && kcm.settings.defaultUser != "")
                            }

                            Accessible.name: i18n("Automatically log in with a specific user and session")
                        }

                        QQC2.ComboBox {
                            id: autologinUser

                            implicitWidth: Kirigami.Units.gridUnit * 12

                            model: kcm.userModel
                            textRole: "display"
                            valueRole: "name"

                            currentValue: kcm.settings.user
                            onActivated: kcm.settings.user = currentValue

                            KCM.SettingStateBinding {
                                visible: autologinBox.checked
                                configObject: kcm.settings
                                settingName: "User"
                                extraEnabledConditions: autologinBox.checked
                            }

                            Accessible.description: i18n("Select a user for automatic log in")
                        }
                    }

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            id: autoLoginSessionLabel
                            Layout.preferredWidth: Math.max(width, autoLoginSessionLabel.width)
                            leftPadding: autologinBox.indicator.width + autologinBox.spacing

                            text: i18nc("@label:listbox part of a sentence: '[Automatically log in as user: foo / ] with session: [bar]'", "with session:")
                        }

                        QQC2.ComboBox {
                            id: autologinSession

                            implicitWidth: Kirigami.Units.gridUnit * 12

                            model: kcm.sessionModel
                            textRole: "display"
                            valueRole: "fileName"

                            currentValue: kcm.settings.session
                            onActivated: kcm.settings.session = currentValue

                            KCM.SettingStateBinding {
                                visible: autologinBox.checked
                                configObject: kcm.settings
                                settingName: "Session"
                                extraEnabledConditions: autologinBox.checked
                            }

                            Accessible.description: i18n("Select a session for automatic log in")
                        }
                    }

                    Kirigami.InlineMessage {
                        id: autologinMessage

                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.largeSpacing - Kirigami.Units.smallSpacing // account for existing layout spacing

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
                }
            }

            Kirigami.FormEntry {
                contentItem: QQC2.CheckBox {
                    text: i18nc("@option:check", "Log in again immediately after logging off")
                    checked: kcm.settings.relogin
                    onToggled: kcm.settings.relogin = checked

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "Relogin"
                        extraEnabledConditions: autologinBox.checked
                    }
                }
            }
        }

        Kirigami.FormGroup {
            title: i18nc("@title:group", "Defaults")

            Kirigami.FormEntry {
                title: i18n("Default user:")

                contentItem: QQC2.RadioButton {
                    QQC2.ButtonGroup.group: QQC2.ButtonGroup {
                        id: preselectedUserGroup
                    }

                    autoExclusive: false
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
            }

            Kirigami.FormEntry {
                contentItem: RowLayout {
                    Kirigami.FormData.buddyFor: customPreselectedUserRadioButton

                    spacing: Kirigami.Units.smallSpacing

                    QQC2.RadioButton {
                        id: customPreselectedUserRadioButton
                        Layout.maximumWidth: indicator.width
                        QQC2.ButtonGroup.group: preselectedUserGroup

                        autoExclusive: false
                        checked: kcm.settings.preselectedUser != ""
                        onToggled: {
                            if (checked) {
                                kcm.settings.preselectedUser = customPreselectedUserComboBox.indexOfValue(kcm.currentUser) !== -1
                                    ? kcm.currentUser
                                    : customPreselectedUserComboBox.valueAt(0);
                            }
                        }
                        onClicked: customPreselectedUserComboBox.popup.open()

                        KCM.SettingHighlighter {
                            highlight: kcm.settings.preselectedUser != kcm.settings.defaultPreselectedUser
                        }
                    }

                    QQC2.ComboBox {
                        id: customPreselectedUserComboBox

                        implicitWidth: Kirigami.Units.gridUnit * 12

                        model: kcm.userModel
                        textRole: "display"
                        valueRole: "name"

                        currentValue: kcm.settings.preselectedUser
                        onActivated: kcm.settings.preselectedUser = currentValue

                        KCM.SettingStateBinding {
                            visible: customPreselectedUserRadioButton.checked
                            configObject: kcm.settings
                            settingName: "PreselectedUser"
                            extraEnabledConditions: customPreselectedUserRadioButton.checked
                        }

                        Accessible.description: i18n("Select the default user to be shown at the login screen")
                    }
                }
            }

            Kirigami.FormSeparator {}

            Kirigami.FormEntry {
                title: i18n("Default session:")

                contentItem: QQC2.RadioButton {
                    QQC2.ButtonGroup.group: QQC2.ButtonGroup {
                        id: preselectedSessionGroup
                    }

                    autoExclusive: false
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
            }

            Kirigami.FormEntry {
                contentItem: RowLayout {
                    Kirigami.FormData.buddyFor: customPreselectedSessionRadioButton

                    spacing: Kirigami.Units.smallSpacing

                    QQC2.RadioButton {
                        id: customPreselectedSessionRadioButton
                        Layout.maximumWidth: indicator.width
                        QQC2.ButtonGroup.group: preselectedSessionGroup

                        autoExclusive: false
                        checked: kcm.settings.preselectedSession != ""
                        onToggled: {
                            if (checked) {
                                kcm.settings.preselectedSession = customPreselectedSessionComboBox.valueAt(0);
                            }
                        }
                        onClicked: customPreselectedSessionComboBox.forceActiveFocus()

                        KCM.SettingHighlighter {
                            highlight: kcm.settings.preselectedSession != kcm.settings.defaultPreselectedSession
                        }
                    }

                    QQC2.ComboBox {
                        id: customPreselectedSessionComboBox

                        implicitWidth: Kirigami.Units.gridUnit * 12

                        model: kcm.sessionModel
                        textRole: "display"
                        valueRole: "fileName"

                        currentValue: kcm.settings.preselectedSession
                        onActivated: kcm.settings.preselectedSession = currentValue

                        KCM.SettingStateBinding {
                            visible: customPreselectedSessionRadioButton.checked
                            configObject: kcm.settings
                            settingName: "PreselectedSession"
                            extraEnabledConditions: customPreselectedSessionRadioButton.checked
                        }

                        Accessible.description: i18n("Select the default session to be shown at the login screen")
                    }
                }
            }
        }
    }
}
