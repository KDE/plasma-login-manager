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

#if defined(Q_OS_FREEBSD)
#include <login_cap.h>
#include <sys/types.h>
#endif

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
#if defined(Q_OS_FREEBSD)
        /* get additional environment variables via setclassenvironment();
            this needs to be done here instead of in UserSession::setupChildProcess
            as the environment for execve() is prepared here
        */
        login_cap_t *lc;

        if (lc = login_getpwclass(pw)) {
            // save, clear and later restore PLASMALOGIN's environment because
            // setclassenvironment() mangles it
            QProcessEnvironment savedEnv = QProcessEnvironment::systemEnvironment();
            QProcessEnvironment::systemEnvironment().clear();
            QString savedLang = env.value(QStringLiteral("LANG"));

            // setclassenvironment() is the implementation inside setusercontext()
            // so use lowest-level function there
            setclassenvironment(lc, pw, 1); /* path variables */
            setclassenvironment(lc, pw, 0); /* non-path variables */
            login_close(lc);
            if (lc = login_getuserclass(pw)) {
                setclassenvironment(lc, pw, 1);
                setclassenvironment(lc, pw, 0);
                login_close(lc);
            }

            // copy all environment variables that are now set
            env.insert(QProcessEnvironment::systemEnvironment());
            // for plasmalogin itself, we don't want to set LANG from capabilities.
            // instead, honour plasmalogin_lang variable from rc script
            if (qobject_cast<HelperApp *>(parent())->user() == QStringLiteral("plasmalogin"))
                env.insert(QStringLiteral("LANG"), savedLang);
            // finally, restore original helper environment
            QProcessEnvironment::systemEnvironment().clear();
            QProcessEnvironment::systemEnvironment().insert(savedEnv);
        }
#endif
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
