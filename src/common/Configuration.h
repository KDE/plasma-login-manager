/*
 * PLASMALOGIN configuration
 * Copyright (C) 2014 Martin Bříza <mbriza@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef PLASMALOGIN_CONFIGURATION_H
#define PLASMALOGIN_CONFIGURATION_H

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QStringList>
#include <QtCore/QDir>
#include <pwd.h>

#include "Constants.h"

#include "ConfigReader.h"

namespace PLASMALOGIN {
    //     Name        File         Sections and/or Entries (but anything else too, it's a class) - Entries in a Config are assumed to be in the General section
    Config(MainConfig, QStringLiteral(CONFIG_FILE), QStringLiteral(CONFIG_DIR), QStringLiteral(SYSTEM_CONFIG_DIR),
        enum NumState { NUM_NONE, NUM_SET_ON, NUM_SET_OFF };

        //  Name                   Type         Default value                                   Description
        // TODO: Change default to x11-user in a future release
        Entry(DisplayServer,       QString,     _S("x11"),                                      _S("Which display server should be used.\n"
                                                                                                   "Valid values are: x11, x11-user, wayland. Wayland support is experimental"));
        Entry(Numlock,             NumState,    NUM_NONE,                                       _S("Initial NumLock state. Can be on, off or none.\n"
                                                                                                   "If property is set to none, numlock won't be changed\n"
                                                                                                   "NOTE: Currently ignored if autologin is enabled."));
        Entry(Namespaces,          QStringList, QStringList(),                                  _S("Comma-separated list of Linux namespaces for user session to enter"));

        // TODO: Not absolutely sure if everything belongs here. Xsessions, VT and probably some more seem universal
        Section(X11,
            Entry(ServerPath,          QString,     _S("/usr/bin/X"),                           _S("Path to X server binary"));
            Entry(ServerArguments,     QString,     _S("-nolisten tcp"),                        _S("Arguments passed to the X server invocation"));
            Entry(XephyrPath,          QString,     _S("/usr/bin/Xephyr"),                      _S("Path to Xephyr binary"));
            Entry(SessionCommand,      QString,     _S(SESSION_COMMAND),                        _S("Path to a script to execute when starting the desktop session"));
            Entry(SessionLogFile,      QString,     _S(".local/share/plasmalogin/xorg-session.log"),   _S("Path to the user session log file"));
            Entry(DisplayCommand,      QString,     _S(DATA_INSTALL_DIR "/scripts/Xsetup"),     _S("Path to a script to execute when starting the display server"));
            Entry(DisplayStopCommand,  QString,     _S(DATA_INSTALL_DIR "/scripts/Xstop"),      _S("Path to a script to execute when stopping the display server"));
        );

        Section(Wayland,
            Entry(SessionDir,          QStringList, {_S("/usr/local/share/wayland-sessions"),
                                                     _S("/usr/share/wayland-sessions")},        _S("Comma-separated list of directories containing available Wayland sessions"));
            Entry(SessionCommand,      QString,     _S(WAYLAND_SESSION_COMMAND),                _S("Path to a script to execute when starting the desktop session"));
            Entry(SessionLogFile,      QString,     _S(".local/share/plasmalogin/wayland-session.log"),_S("Path to the user session log file"));
        );

        Section(Users,
            Entry(DefaultPath,         QString,     _S("/usr/local/bin:/usr/bin:/bin"),         _S("Default $PATH for logged in users"));
            Entry(MinimumUid,          int,         UID_MIN,                                    _S("Minimum user id for displayed users"));
            Entry(MaximumUid,          int,         UID_MAX,                                    _S("Maximum user id for displayed users"));
            Entry(HideUsers,           QStringList, QStringList(),                              _S("Comma-separated list of users that should not be listed"));
            Entry(HideShells,          QStringList, QStringList(),                              _S("Comma-separated list of shells.\n"
                                                                                                   "Users with these shells as their default won't be listed"));
            Entry(RememberLastUser,    bool,        true,                                       _S("Remember the last successfully logged in user"));
            Entry(RememberLastSession, bool,        true,                                       _S("Remember the session of the last successfully logged in user"));

            Entry(ReuseSession,        bool,        true,                                       _S("When logging in as the same user twice, restore the original session, rather than create a new one"));
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

    extern MainConfig mainConfig;
    extern StateConfig stateConfig;

    inline QTextStream& operator>>(QTextStream &str, MainConfig::NumState &state) {
        QString text = str.readLine().trimmed();
        if (text.compare(QLatin1String("on"), Qt::CaseInsensitive) == 0)
            state = MainConfig::NUM_SET_ON;
        else if (text.compare(QLatin1String("off"), Qt::CaseInsensitive) == 0)
            state = MainConfig::NUM_SET_OFF;
        else
            state = MainConfig::NUM_NONE;
        return str;
    }

    inline QTextStream& operator<<(QTextStream &str, const MainConfig::NumState &state) {
        if (state == MainConfig::NUM_SET_ON)
            str << "on";
        else if (state == MainConfig::NUM_SET_OFF)
            str << "off";
        else
            str << "none";
        return str;
    }
}

#endif // PLASMALOGIN_CONFIGURATION_H
