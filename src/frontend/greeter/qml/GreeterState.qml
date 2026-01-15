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

    // TODO: In Login.qml, identify which LoginState we represent and have connections on GreeterState
    //       property changed and update GreeterState property when the relevant thing is changed
    //       also property for session combo

    readonly property int beyondUserLimit: PlasmaLogin.UserModel.rowCount() > 7

    property int loginState: GreeterState.LoginState.UserList

    property int sessionIndex: {
        // indexOfData will return -1 if passed an empty string, which these are by default
        let preselectedSessionIndex = PlasmaLogin.SessionModel.indexOfData(PlasmaLogin.Settings.preselectedSession, PlasmaLogin.SessionModel.PathRole);
        let lastLoggedInSessionIndex = PlasmaLogin.SessionModel.indexOfData(PlasmaLogin.StateConfig.lastLoggedInSession, PlasmaLogin.SessionModel.PathRole);

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

    function activateWindow(window): void {
        if (!window) {
            return;
        }

        internal.activeWindow = window;
    }

    function timeoutWindow(window): void {
        if (internal.activeWindow == window) {
            internal.activeWindow = null;
        }
    }

    property string lastLoggedInUser
    property string lastLoggedInSession

    function handleLoginRequest(username, password, sessionType, sessionFileName, sessionPath) {
        greeterState.lastLoggedInUser = username;
        greeterState.lastLoggedInSession = sessionPath;

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
