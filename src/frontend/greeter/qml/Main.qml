/*
 * SPDX-FileCopyrightText: Oliver Beard
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import Qt5Compat.GraphicalEffects

import org.kde.kirigami as Kirigami
import org.kde.breeze.components as BreezeComponents
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.private.keyboardindicator as KeyboardIndicator

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

    Connections {
        target: greeterEventFilter

        function onKeyPressed(): void {
            // callLater, as otherwise 'enter' key press would arrive after waking
            // and the uiVisible check would pass and a login attempt would be made
            Qt.callLater(() => PlasmaLogin.GreeterState.activateWindow(loginScreenRoot.Window.window));
        }

        function onEscapeKeyPressed(): void {
            PlasmaLogin.GreeterState.timeoutWindow(loginScreenRoot.Window.window);
            /*
-            if (inputPanel.keyboardActive) {
-                inputPanel.showHide();
-            }
-            */
            PlasmaLogin.GreeterState.clearPasswords();
        }
    }

    MouseArea {
        id: loginScreenRoot
        anchors.fill: parent

        hoverEnabled: true

        property bool uiVisible: PlasmaLogin.GreeterState.activeWindow === Window.window

        cursorShape: uiVisible ? Qt.ArrowCursor : Qt.BlankCursor

        onPressed: PlasmaLogin.GreeterState.activateWindow(Window.window);
        onPositionChanged: PlasmaLogin.GreeterState.activateWindow(Window.window);

        DropShadow {
            id: clockShadow
            anchors.fill: clock
            source: clock
            visible: !softwareRendering && clock.visible
            radius: 7
            verticalOffset: 0.8
            samples: 15
            spread: 0.2
            color: Qt.rgba(0, 0, 0, 0.7)
            opacity: loginScreenRoot.uiVisible ? 0 : 1
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
            visible: y > 0 && Settings.showClock
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

            opacity: loginScreenRoot.uiVisible ? 1 : 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.longDuration
                }
            }

            Connections {
                target: PlasmaLogin.GreeterState

                function onLoginStateChanged() {
                    switch (PlasmaLogin.GreeterState.loginState) {
                        case PlasmaLogin.GreeterState.LoginState.UserList:
                            if (mainStack.depth !== 2) { return; /* already showing user list */ }
                            mainStack.pop();
                            return;
                        case PlasmaLogin.GreeterState.LoginState.UserPrompt:
                            if (mainStack.depth !== 1) { return; /* already showing user prompt */ }
                            mainStack.push(userPromptComponent);
                            return;
                        default:
                            console.warn("Cannot synchronize login state:", PlasmaLogin.GreeterState.loginState);
                    }
                }
            }

            initialItem: Login {
                id: userListComponent
                userListModel: PlasmaLogin.UserModel
                loginScreenUiVisible: loginScreenRoot.uiVisible
                userListCurrentIndex: {
                    // indexOfData will return -1 if passed an empty string, which these are by default
                    let preselectedUserIndex = PlasmaLogin.UserModel.indexOfData(PlasmaLogin.Settings.preselectedUser, PlasmaLogin.UserModel.NameRole);
                    let lastLoggedInUserIndex = PlasmaLogin.UserModel.indexOfData(PlasmaLogin.StateConfig.lastLoggedInUser, PlasmaLogin.UserModel.NameRole);

                    if (preselectedUserIndex != -1) {
                        return preselectedUserIndex;
                    } else if (lastLoggedInUserIndex != -1) {
                        return lastLoggedInUserIndex;
                    } else {
                        return 0;
                    }
                }

                showUserList: !PlasmaLogin.GreeterState.beyondUserLimit

                notificationMessage: {
                    const parts = [];
                    if (capsLockState.locked) {
                        parts.push(i18nd("plasma_login", "Caps Lock is on"));
                    }
                    if (root.notificationMessage) {
                        parts.push(root.notificationMessage);
                    }
                    return parts.join(" • ");
                }

                //actionItemsVisible: !inputPanel.keyboardActive
                actionItems: [
                    BreezeComponents.ActionButton {
                        icon.name: "system-hibernate"
                        text: i18ndc("plasma_login", "Suspend to disk", "Hibernate")
                        visible: PlasmaLogin.SessionManagement.canHibernate
                        onClicked: {
                            PlasmaLogin.GreeterState.clearPasswords();
                            PlasmaLogin.SessionManagement.hibernate();
                        }
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-suspend"
                        text: i18ndc("plasma_login", "Suspend to RAM", "Sleep")
                        visible: PlasmaLogin.SessionManagement.canSuspend
                        onClicked: {
                            PlasmaLogin.GreeterState.clearPasswords();
                            PlasmaLogin.SessionManagement.suspend();
                        }
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-reboot"
                        text: i18nd("plasma_login", "Restart")
                        visible: PlasmaLogin.SessionManagement.canReboot
                        onClicked: PlasmaLogin.SessionManagement.requestReboot(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-shutdown"
                        text: i18nd("plasma_login", "Shut Down")
                        visible: PlasmaLogin.SessionManagement.canShutdown
                        onClicked: PlasmaLogin.SessionManagement.requestShutdown(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-user-prompt"
                        text: i18ndc("plasma_login", "For switching to a username and password prompt", "Other…")
                        onClicked: PlasmaLogin.GreeterState.loginState = PlasmaLogin.GreeterState.LoginState.UserPrompt
                        visible: !userListComponent.showUsernamePrompt
                    }]

                onLoginRequest: (username, password) => root.handleLoginRequest(username, password, sessionButton.currentSessionType, sessionButton.currentSessionFileName)
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
                loginScreenUiVisible: loginScreenRoot.uiVisible
                fontSize: Kirigami.Theme.defaultFont.pointSize + 2

                notificationMessage: {
                    const parts = [];
                    if (capsLockState.locked) {
                        parts.push(i18nd("plasma_login", "Caps Lock is on"));
                    }
                    if (root.notificationMessage) {
                        parts.push(root.notificationMessage);
                    }
                    return parts.join(" • ");
                }

                // using a model rather than a QObject list to avoid QTBUG-75900
                userListModel: ListModel {
                    ListElement {
                        realName: ""
                        icon: ""
                    }
                    Component.onCompleted: {
                        // as we can't bind inside ListElement
                        setProperty(0, "realName", i18nd("plasma_login", "Type in Username and Password"));
                        setProperty(0, "icon", Qt.resolvedUrl(".face.icon").toString());
                    }
                }

                onLoginRequest: (username, password) => root.handleLoginRequest(username, password, sessionButton.currentSessionType, sessionButton.currentSessionFileName)

                //actionItemsVisible: !inputPanel.keyboardActive
                actionItems: [
                    BreezeComponents.ActionButton {
                        icon.name: "system-hibernate"
                        text: i18ndc("plasma_login", "Suspend to disk", "Hibernate")
                        visible: PlasmaLogin.SessionManagement.canHibernate
                        onClicked: {
                            PlasmaLogin.GreeterState.clearPasswords();
                            PlasmaLogin.SessionManagement.hibernate();
                        }
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-suspend"
                        text: i18ndc("plasma_login", "Suspend to RAM", "Sleep")
                        visible: PlasmaLogin.SessionManagement.canSuspend
                        onClicked: {
                            PlasmaLogin.GreeterState.clearPasswords();
                            PlasmaLogin.SessionManagement.suspend();
                        }
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-reboot"
                        text: i18nd("plasma_login", "Restart")
                        visible: PlasmaLogin.SessionManagement.canReboot
                        onClicked: PlasmaLogin.SessionManagement.requestReboot(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-shutdown"
                        text: i18nd("plasma_login", "Shut Down")
                        visible: PlasmaLogin.SessionManagement.canShutdown
                        onClicked: PlasmaLogin.SessionManagement.requestShutdown(PlasmaLogin.SessionManagement.ConfirmationMode.Skip)
                    },
                    BreezeComponents.ActionButton {
                        icon.name: "system-user-list"
                        text: i18nd("plasma_login", "List Users")
                        onClicked: PlasmaLogin.GreeterState.loginState = PlasmaLogin.GreeterState.LoginState.UserList
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

    function handleLoginRequest(username, password, sessionType, sessionFileName) {
        root.notificationMessage = "";
        // GreeterState handles updating stateconfig with user/session of successful login
        PlasmaLogin.GreeterState.handleLoginRequest(username, password, sessionType, sessionFileName);
    }

    Connections {
        target: PlasmaLogin.Authenticator

        function onLoginFailed() {
            notificationMessage = i18nd("plasma_login", "Login Failed");

            footer.enabled = true;
            mainStack.enabled = true;
            userListComponent.userList.opacity = 1;

            rejectPasswordAnimation.start();
        }

        function onLoginSucceeded() {
            mainStack.opacity = 0;
            footer.opacity = 0;
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
