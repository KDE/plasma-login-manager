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
