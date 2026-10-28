// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#pragma once

#include "Login1Session.h"

namespace PLASMALOGIN
{

class Display;

//! A greeter session represents a logind session used for our greeter. Every Seat should generally have one of these.
class GreeterSession
{
public:
    GreeterSession(const QDBusObjectPath &path);

    //! Activates the greeter session.
    void activate();

    //! Sends SIGTERM to the session.
    void sigterm();

    /*!
        Forcefully terminate the session.

        It's not defined what forcefully means but it seems to just outright kill.
        Because of this you should only use this after having asked nicely using a sigterm.
    */
    void terminate();

private:
    OrgFreedesktopLogin1SessionInterface m_interface;
};

} // namespace PLASMALOGIN
