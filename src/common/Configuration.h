/*
 * PLASMALOGIN configuration
 * SPDX-FileCopyrightText: 2014 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef PLASMALOGIN_CONFIGURATION_H
#define PLASMALOGIN_CONFIGURATION_H

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <pwd.h>

#include "Constants.h"

#include "ConfigReader.h"

namespace PLASMALOGIN
{
// clang-format off
    //     Name        File         Sections and/or Entries (but anything else too, it's a class) - Entries in a Config are assumed to be in the General section
    Config(MainConfig, QStringLiteral(CONFIG_FILE), QStringLiteral(CONFIG_DIR), QStringLiteral(SYSTEM_CONFIG_DIR),
        //  Name                   Type         Default value                                   Description
        Entry(Namespaces,          QStringList, QStringList(),                                  _S("Comma-separated list of Linux namespaces for user session to enter"));
        // TODO: Not absolutely sure if everything belongs here. Xsessions, VT and probably some more seem universal
        Section(X11,
            Entry(ServerPath,          QString,     _S("/usr/bin/X"),                           _S("Path to X server binary"));
            Entry(ServerArguments,     QString,     _S("-nolisten tcp"),                        _S("Arguments passed to the X server invocation"));
            Entry(SessionLogFile,      QString,     _S(".local/share/plasmalogin/xorg-session.log"),   _S("Path to the user session log file"));
        );

        Section(Wayland,
            Entry(SessionLogFile,      QString,     _S(".local/share/plasmalogin/wayland-session.log"),_S("Path to the user session log file"));
        );

        Section(Users,
            Entry(DefaultPath,         QString,     _S("/usr/local/bin:/usr/bin:/bin"),         _S("Default $PATH for logged in users"));
            Entry(RememberLastUser,    bool,        true,                                       _S("Remember the last successfully logged in user"));
        );

        Section(Autologin,
            Entry(User,                QString,     QString(),                                  _S("Username for autologin session"));
            Entry(Session,             QString,     QString(),                                  _S("Name of session file for autologin session (if empty try last logged in)"));
            Entry(Relogin,             bool,        false,                                      _S("Whether plasmalogin should automatically log back into sessions when they exit"));
        );
    );

    Config(StateConfig, []()->QString{auto tmp = getpwnam("plasmalogin"); return tmp ? QString::fromLocal8Bit(tmp->pw_dir) : QStringLiteral(STATE_DIR);}().append(QStringLiteral("/state.conf")), QString(), QString(),
        Section(Last,
            Entry(Session,         QString,     QString(),                                      _S("Name of the session for the last logged-in user.\n"
                                                                                                   "This session will be preselected when the login screen appears."));
            Entry(User,            QString,     QString(),                                      _S("Name of the last logged-in user.\n"
                                                                                                   "This user will be preselected when the login screen appears"));
        );
    );
// clang-format on

extern MainConfig mainConfig;
extern StateConfig stateConfig;
}

#endif // PLASMALOGIN_CONFIGURATION_H
