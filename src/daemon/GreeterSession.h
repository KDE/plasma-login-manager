// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#pragma once

#include "Login1Session.h"

namespace PLASMALOGIN
{

class Display;

//! A greeter session represents a logind session used for our greeter.
class GreeterSession
{
public:
    GreeterSession(const QDBusObjectPath &path);

    //! Activates the greeter session.
    void activate();

private:
    OrgFreedesktopLogin1SessionInterface m_interface;
};

} // namespace PLASMALOGIN
