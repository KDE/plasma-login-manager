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

    readonly property int beyondUserLimit: PlasmaLogin.UserModel.rowCount() === 0 || PlasmaLogin.UserModel.rowCount() > 7

    property int loginState: GreeterState.LoginState.UserList

    onLoginStateChanged: {
        clearPasswords();

        if (greeterState.loginState == GreeterState.LoginState.UserList) {
            let user = PlasmaLogin.UserModel.data(PlasmaLogin.UserModel.index(greeterState.userListIndex, 0), PlasmaLogin.UserModel.NameRole);
            updateSessionForUser(user);
        }
    }

    property int sessionIndex: {
        // indexOfData will return -1 if passed an empty string, which these are by default
        let preselectedSessionIndex = PlasmaLogin.SessionModel.indexOfData(PlasmaLogin.Settings.preselectedSession, PlasmaLogin.SessionModel.FileNameRole);
        let lastLoggedInSessionIndex = PlasmaLogin.SessionModel.indexOfData(getLastLoggedInSessionForUser(PlasmaLogin.StateConfig.lastLoggedInUser), PlasmaLogin.SessionModel.FileNameRole);

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

    onUserListIndexChanged: {
        let user = PlasmaLogin.UserModel.data(PlasmaLogin.UserModel.index(greeterState.userListIndex, 0), PlasmaLogin.UserModel.NameRole);
        updateSessionForUser(user);
    }

    property string userListPassword: ""

    property string userPromptUsername: ""
    property string userPromptPassword: ""

    onUserPromptUsernameChanged: {
        updateSessionForUser(greeterState.userPromptUsername);
    }

    property bool showPassword: false

    // Shared functionality

    property bool greeterHasSessionButtonPopup: false
    readonly property bool inhibitGreeterTimeout: {
        if (greeterState.loginState === PlasmaLogin.GreeterState.LoginState.UserList && greeterState.userListPassword.length > 0) {
            // We're on the user list and a password is entered
            return true;
        } else if (greeterState.loginState === PlasmaLogin.GreeterState.UserPrompt && greeterState.userPromptPassword.length > 0) {
            // We're on the user prompt and a password is entered
            return true;
        } else if (greeterState.greeterHasSessionButtonPopup) {
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

    function handleLoginRequest(username, password) {
        greeterState.lastLoggedInUser = username;

        let sessionType = PlasmaLogin.SessionModel.data(PlasmaLogin.SessionModel.index(greeterState.sessionIndex, 0), PlasmaLogin.SessionModel.TypeRole)
        let sessionFileName = PlasmaLogin.SessionModel.data(PlasmaLogin.SessionModel.index(greeterState.sessionIndex, 0), PlasmaLogin.SessionModel.FileNameRole)
        greeterState.lastLoggedInSession = sessionFileName;

        PlasmaLogin.Authenticator.login(username, password, sessionType, sessionFileName);
    }

    property var lastLoggedInSessions: {
        let lastLoggedInUser = PlasmaLogin.StateConfig.lastLoggedInUser;
        let sessionsJson = PlasmaLogin.StateConfig.lastLoggedInSession;
        let result = {};
        if (sessionsJson !== "") {
            try {
                result = JSON.parse(sessionsJson);
            } catch (e) {
                if (lastLoggedInUser !== "") {
                    result[lastLoggedInUser] = sessionsJson;
                }
            }
        }

        for (let user in result) {
            if (PlasmaLogin.UserModel.indexOfData(user, PlasmaLogin.UserModel.NameRole) === -1) {
                delete result[user];
            }
        }
        
        return result;
    }

    function getLastLoggedInSessionForUser(username) {
        return greeterState.lastLoggedInSessions[username] ?? -1;
    }

    function setLastLoggedInSessionForUser(username, sessionPath) {
        greeterState.lastLoggedInSessions[username] = sessionPath;
        PlasmaLogin.StateConfig.lastLoggedInSession = JSON.stringify(greeterState.lastLoggedInSessions);
    }

    function updateSessionForUser(username) {
        let session = getLastLoggedInSessionForUser(username);
        let lastLoggedInSessionIndex = PlasmaLogin.SessionModel.indexOfData(session, PlasmaLogin.SessionModel.FileNameRole);
        if (lastLoggedInSessionIndex != -1) {
            greeterState.sessionIndex = lastLoggedInSessionIndex;
        }
    }

    Connections {
        target: PlasmaLogin.Authenticator

        function onLoginSucceeded() {
            PlasmaLogin.StateConfig.lastLoggedInUser = greeterState.lastLoggedInUser;
            setLastLoggedInSessionForUser(greeterState.lastLoggedInUser, greeterState.lastLoggedInSession);
            PlasmaLogin.StateConfig.save();
        }

        function onLoginFailed() {
            clearPasswords();
        }
    }
}
