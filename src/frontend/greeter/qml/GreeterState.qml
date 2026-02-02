/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma Singleton

import QtQuick

import org.kde.plasma.login as PlasmaLogin

Item {
    id: greeterState

    enum LoginState {
        UserList = 0,
        UserPrompt = 1
    }

    QtObject {
        id: internal

        property var activeWindow: null
    }

    Binding {
        target: PlasmaLogin.BlurScreenBridge
        property: "activeWindow"
        value: internal.activeWindow
    }

    readonly property var activeWindow: internal.activeWindow

    // Shared state

    readonly property int beyondUserLimit: PlasmaLogin.UserModel.rowCount() > 7

    property int loginState: GreeterState.LoginState.UserList
    onLoginStateChanged: clearPasswords();

    property int sessionIndex: {
        // indexOfData will return -1 if passed an empty string, which these are by default
        let preselectedSessionIndex = PlasmaLogin.SessionModel.indexOfData(PlasmaLogin.Settings.preselectedSession, PlasmaLogin.SessionModel.FileNameRole);
        let lastLoggedInSessionIndex = PlasmaLogin.SessionModel.indexOfData(PlasmaLogin.StateConfig.lastLoggedInSession, PlasmaLogin.SessionModel.FileNameRole);

        if (preselectedSessionIndex != -1) {
            return preselectedSessionIndex;
        } else if (lastLoggedInSessionIndex != -1) {
            return lastLoggedInSessionIndex;
        } else {
            return 0;
        }
    }

    property int userListIndex: {
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
    property string userListPassword: ""

    property string userPromptUsername: ""
    property string userPromptPassword: ""

    property bool showPassword: false

    // Shared functionality

    readonly property bool inhibitGreeterTimeout: {
        if (greeterState.loginState === PlasmaLogin.GreeterState.LoginState.UserList && greeterState.userListPassword.length > 0) {
            // We're on the user list and a password is entered
            return true;
        } else if (greeterState.loginState === PlasmaLogin.GreeterState.UserPrompt && greeterState.userPromptPassword.length > 0) {
            // We're on the user prompt and a password is entered
            return true;
        }

        // inputPanel.keyboardActive

        // No reason to block timeout
        return false;
    }
    onInhibitGreeterTimeoutChanged: {
        let greeterShouldTimeOut = greeterState.activeWindow !== null && !greeterState.inhibitGreeterTimeout;
        if (greeterTimeoutTimer.running !== greeterShouldTimeOut) {
            greeterTimeoutTimer.running = greeterShouldTimeOut;
        }
    }

    Timer {
        id: greeterTimeoutTimer
        running: false
        interval: 10000
        onTriggered: {
            if (internal.activeWindow) {
                greeterState.showPassword = false;
                timeoutWindow(internal.activeWindow);
            }
        }
    }

    function clearPasswords(): void {
        userListPassword = "";
        userPromptPassword = "";
    }

    function activateWindow(window): void {
        if (!window) {
            return;
        }

        internal.activeWindow = window;

        window.requestActivate();

        if (!inhibitGreeterTimeout) {
            greeterTimeoutTimer.restart();
        }
    }

    function timeoutWindow(window): void {
        if (internal.activeWindow == window) {
            internal.activeWindow = null;
        }

        greeterTimeoutTimer.stop();
    }

    // Remember last logged in user/session

    property string lastLoggedInUser
    property string lastLoggedInSession

    function handleLoginRequest(username, password, sessionType, sessionFileName) {
        greeterState.lastLoggedInUser = username;
        greeterState.lastLoggedInSession = sessionFileName;

        PlasmaLogin.Authenticator.login(username, password, sessionType, sessionFileName);
    }

    Connections {
        target: PlasmaLogin.Authenticator

        function onLoginSucceeded() {
            PlasmaLogin.StateConfig.lastLoggedInUser = greeterState.lastLoggedInUser;
            PlasmaLogin.StateConfig.lastLoggedInSession = greeterState.lastLoggedInSession;
            PlasmaLogin.StateConfig.save();
        }
    }
}
