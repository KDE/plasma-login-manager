/*
 * Base backend class to be inherited further
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "Backend.h"
#include "HelperApp.h"

#include "UserSession.h"
#include "backend/PamBackend.h"

#include <QtCore/QProcessEnvironment>

#include <pwd.h>

namespace PLASMALOGIN
{
Backend::Backend(HelperApp *parent)
    : QObject(parent)
    , m_app(parent)
{
}

Backend *Backend::get(HelperApp *parent)
{
    return new PamBackend(parent);
}

void Backend::setAutologin(bool on)
{
    m_autologin = on;
}

void Backend::setDisplayServer(bool on)
{
    m_displayServer = on;
}

void Backend::setGreeter(bool on)
{
    m_greeter = on;
}

bool Backend::openSession()
{
    QProcessEnvironment env = m_app->session()->processEnvironment();
    struct passwd *pw;
    pw = getpwnam(qPrintable(qobject_cast<HelperApp *>(parent())->user()));
    if (pw) {
        env.insert(QStringLiteral("HOME"), QString::fromLocal8Bit(pw->pw_dir));
        env.insert(QStringLiteral("PWD"), QString::fromLocal8Bit(pw->pw_dir));
        env.insert(QStringLiteral("SHELL"), QString::fromLocal8Bit(pw->pw_shell));
        env.insert(QStringLiteral("USER"), QString::fromLocal8Bit(pw->pw_name));
        env.insert(QStringLiteral("LOGNAME"), QString::fromLocal8Bit(pw->pw_name));
    }
    if (env.value(QStringLiteral("XDG_SESSION_CLASS")) == QLatin1String("greeter")) {
        // Qt internally may load the xdg portal system early on, prevent this, we do not have a functional session running.
        env.insert(QStringLiteral("QT_NO_XDG_DESKTOP_PORTAL"), QStringLiteral("1"));
    }
    // TODO: I'm fairly sure this shouldn't be done for PAM sessions, investigate!
    m_app->session()->setProcessEnvironment(env);
    return m_app->session()->start();
}

bool Backend::closeSession()
{
    return true;
}
}

#include "moc_Backend.cpp"
