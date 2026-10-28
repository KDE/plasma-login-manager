// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include "GreeterSession.h"

#include <csignal>

namespace PLASMALOGIN
{

GreeterSession::GreeterSession(const QDBusObjectPath &path)
    : m_interface(Logind::serviceName(), path.path(), QDBusConnection::systemBus())
{
}

void GreeterSession::activate()
{
    m_interface.Activate();
}

void GreeterSession::sigterm()
{
    m_interface.Kill("all", SIGTERM);
}

void GreeterSession::terminate()
{
    m_interface.Terminate();
}

} // namespace PLASMALOGIN
