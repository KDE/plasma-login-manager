import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami
import org.kde.breeze.components as BreezeComponents
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.private.keyboardindicator as KeyboardIndicator

import org.greetd as Greet
import org.kde.plasma.login as PlasmaLogin

Item {
    id: root
    anchors.fill: parent

    // If we're using software rendering, draw outlines instead of shadows
    // See https://bugs.kde.org/show_bug.cgi?id=398317
    readonly property bool softwareRendering: GraphicsInfo.api === GraphicsInfo.Software

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    property string notificationMessage

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    KeyboardIndicator.KeyState {
        id: capsLockState
        key: Qt.Key_CapsLock
    }

    BreezeComponents.RejectPasswordAnimation {
        id: rejectPasswordAnimation
        target: mainStack
    }

    MouseArea {
        id: loginScreenRoot
        anchors.fill: parent

        property bool uiVisible: true
        property bool blockUI: mainStack.depth > 1 || userListComponent.mainPasswordBox.text.length > 0 // || inputPanel.keyboardActive || config.type !== "image"

        hoverEnabled: true
        onPressed: uiVisible = true;
        onPositionChanged: uiVisible = true;
        onUiVisibleChanged: {
            if (blockUI) {
                fadeoutTimer.running = false;
            } else if (uiVisible) {
                fadeoutTimer.restart();
            }
        }
        onBlockUIChanged: {
            if (blockUI) {
                fadeoutTimer.running = false;
                uiVisible = true;
            } else if (uiVisible) {
                fadeoutTimer.restart();
            }
        }

        Keys.onPressed: (event) => {
            uiVisible = true;
            event.accepted = true;
        }

        Timer {
            id: fadeoutTimer
            running: true
            interval: 60000
            onTriggered: {
                if (!loginScreenRoot.blockUI) {
                    userListComponent.mainPasswordBox.showPassword = false;
                    loginScreenRoot.uiVisible = false;
                }
            }
        }

        DropShadow {
            id: clockShadow
            anchors.fill: clock
            source: clock
            visible: !softwareRendering && clock.visible
            radius: 7
            samples: 15
            spread: 0.3
            color: "black" // shadows should always be black
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.veryLongDuration * 2
                    easing.type: Easing.InOutQuad
                }
            }
        }

        BreezeComponents.Clock {
            id: clock
            property Item shadow: clockShadow
            visible: y > 0
            anchors.horizontalCenter: parent.horizontalCenter
            y: (userListComponent.userList.y + mainStack.y)/2 - height/2
            Layout.alignment: Qt.AlignBaseline
        }

        QQC2.StackView {
            id: mainStack
            anchors.left: parent.left
            anchors.right: parent.right

            height: root.height + Kirigami.Units.gridUnit * 3

            hoverEnabled: true

            focus: true

            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                }
            }

            initialItem: Login {
                id: userListComponent
                userListModel: PlasmaLogin.UserModel
                loginScreenUiVisible: loginScreenRoot.uiVisible
                userListCurrentIndex: PlasmaLogin.UserModel.lastIndex >= 0 ? PlasmaLogin.UserModel.lastIndex : 0
                lastUserName: PlasmaLogin.UserModel.lastUser
                showUserList: {
                    if (!userListModel.hasOwnProperty("count")
                        || !userListModel.hasOwnProperty("disableAvatarsThreshold")) {
                        return false
                    }

                    if (userListModel.count === 0 ) {
                        return false
                    }

                    if (userListModel.hasOwnProperty("containsAllUsers") && !userListModel.containsAllUsers) {
                        return false
                    }

                    return userListModel.count <= userListModel.disableAvatarsThreshold
                }

                notificationMessage: {
                    const parts = [];
                    if (capsLockState.locked) {
                        parts.push(i18nd("plasma-desktop-sddm-theme", "Caps Lock is on"));
                    }
                    if (root.notificationMessage) {
                        parts.push(root.notificationMessage);
                    }
                    return parts.join(" • ");
                }

                //actionItemsVisible: !inputPanel.keyboardActive
                actionItems: [
                    BreezeComponents.ActionButton {
                        icon.name: "system-suspend"
                        text: i18ndc("plasma-desktop-sddm-theme", "Suspend to RAM", "Sleep")
                        enabled: PlasmaLogin.SessionManagement.canSuspend
                        onClicked: PlasmaLogin.SessionManagement.suspend()
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-reboot"
                        text: i18nd("plasma-desktop-sddm-theme", "Restart")
                        enabled: PlasmaLogin.SessionManagement.canReboot
                        onClicked: PlasmaLogin.SessionManagement.requestReboot(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-shutdown"
                        text: i18nd("plasma-desktop-sddm-theme", "Shut Down")
                        enabled: PlasmaLogin.SessionManagement.canShutdown
                        onClicked: PlasmaLogin.SessionManagement.requestShutdown(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-user-prompt"
                        text: i18ndc("plasma-desktop-sddm-theme", "For switching to a username and password prompt", "Other…")
                        onClicked: mainStack.push(userPromptComponent)
                        visible: !userListComponent.showUsernamePrompt
                    }]

                onLoginRequest: (username, password) => {
                    root.notificationMessage = ""
                    /*sddm.login(username, password, sessionButton.currentIndex);*/
                    root.tryLogin(username, password, sessionButton.currentIndex);
                }
            }

            readonly property real zoomFactor: 1.5

            popEnter: Transition {
                ScaleAnimator {
                    from: mainStack.zoomFactor
                    to: 1
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
                OpacityAnimator {
                    from: 0
                    to: 1
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
            }

            popExit: Transition {
                ScaleAnimator {
                    from: 1
                    to: 1 / mainStack.zoomFactor
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
                OpacityAnimator {
                    from: 1
                    to: 0
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
            }

            pushEnter: Transition {
                ScaleAnimator {
                    from: 1 / mainStack.zoomFactor
                    to: 1
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
                OpacityAnimator {
                    from: 0
                    to: 1
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
            }

            pushExit: Transition {
                ScaleAnimator {
                    from: 1
                    to: mainStack.zoomFactor
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
                OpacityAnimator {
                    from: 1
                    to: 0
                    duration: Kirigami.Units.veryLongDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Component {
            id: userPromptComponent

            Login {
                showUsernamePrompt: true
                notificationMessage: root.notificationMessage
                loginScreenUiVisible: loginScreenRoot.uiVisible
                fontSize: Kirigami.Theme.defaultFont.pointSize + 2

                // using a model rather than a QObject list to avoid QTBUG-75900
                userListModel: ListModel {
                    ListElement {
                        name: ""
                        icon: ""
                    }
                    Component.onCompleted: {
                        // as we can't bind inside ListElement
                        setProperty(0, "name", i18nd("plasma-desktop-sddm-theme", "Type in Username and Password"));
                        setProperty(0, "icon", Qt.resolvedUrl("faces/.face.icon"))
                    }
                }

                onLoginRequest: (username, password) => {
                    root.notificationMessage = ""
                    /*sddm.login(username, password, sessionButton.currentIndex);*/
                    root.tryLogin(username, password, sessionButton.currentIndex);
                }

                //actionItemsVisible: !inputPanel.keyboardActive
                actionItems: [
                    BreezeComponents.ActionButton {
                        icon.name: "system-suspend"
                        text: i18ndc("plasma-desktop-sddm-theme", "Suspend to RAM", "Sleep")
                        enabled: PlasmaLogin.SessionManagement.canSuspend
                        onClicked: PlasmaLogin.SessionManagement.suspend()
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-reboot"
                        text: i18nd("plasma-desktop-sddm-theme", "Restart")
                        enabled: PlasmaLogin.SessionManagement.canReboot
                        onClicked: PlasmaLogin.SessionManagement.requestReboot(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-shutdown"
                        text: i18nd("plasma-desktop-sddm-theme", "Shut Down")
                        enabled: PlasmaLogin.SessionManagement.canShutdown
                        onClicked: PlasmaLogin.SessionManagement.requestShutdown(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-user-list"
                        text: i18nd("plasma-desktop-sddm-theme", "List Users")
                        onClicked: mainStack.pop()
                    }
                ]
            }
        }

        RowLayout {
            id: footer
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.smallSpacing

            spacing: Kirigami.Units.smallSpacing

            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                }
            }

            /* Virtual keyboard btn */

            KeyboardButton {
                id: keyboardButton
            }

            SessionButton {
                id: sessionButton

                onSessionChanged: {
                    // NOTE: This won't work for userPromptComponent: we might
                    // want a function to focus the correct password box, but
                    // userPromptComponent has both that and the user box
                    // Perhaps we need to store which one last had focus

                    // Otherwise the password field loses focus and virtual keyboard
                    // keystrokes get eaten
                    userListComponent.mainPasswordBox.forceActiveFocus();
                }

                Layout.fillHeight: true
                containmentMask: Item {
                    parent: sessionButton
                    anchors.fill: parent
                    /*
                    anchors.leftMargin: virtualKeyboardButton.visible || keyboardButton.visible
                        ? 0 : -footer.anchors.margins
                    */
                    anchors.leftMargin: 0
                    anchors.bottomMargin: -footer.anchors.margins
                }
            }

            Item {
                Layout.fillWidth: true
            }

            BreezeComponents.Battery {}
        }
    }

    function tryLogin(username, password, sessionIndex) {
        // TODO: sessionIndex unused
        if (Greet.Authenticator.authenticate(username, password)) {
            // we would then do Autenticator.startSession()
            // probably with an abstraction so we pass the desktop file

            mainStack.opacity = 0
            footer.opacity = 0
        } else {
            notificationMessage = i18nd("plasma-desktop-sddm-theme", "Login Failed")
            footer.enabled = true
            mainStack.enabled = true
            userListComponent.userList.opacity = 1
            rejectPasswordAnimation.start()
        }
    }

    onNotificationMessageChanged: {
        if (notificationMessage) {
            notificationResetTimer.start();
        }
    }

    Timer {
        id: notificationResetTimer
        interval: 3000
        onTriggered: notificationMessage = ""
    }
}

/*
Rectangle {
    anchors.fill: parent

    ColumnLayout {
        anchors.centerIn: parent
        PlasmaComponents.TextField {
            id: usernameField
        }
        PlasmaComponents.TextField {
            id: passwordField
            echoMode: TextInput.Password
        }
        PlasmaComponents.Label {
            id: result
        }
        PlasmaComponents.Button {
            text: "login"
            onClicked: function () {
                result.text = "LOGIN STARTED"
                if (Greet.Authenticator.authenticate(usernameField.text, passwordField.text)) {
                    result.text = "LOGIN SUCCESSFUL"
                    // we would then do Autenticator.startSession()
                    // probably with an abstraction so we pass the desktop file
                } else {
                    result.text = "LOGIN FAILED"
                }
            }
        }
    }
}
*/
