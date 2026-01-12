## SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Abdurrahman AVCI <abdurrahmanavci@gmail.com>
# SPDX-FileCopyrightText: Alberto Milone <alberto.milone@canonical.com>
# SPDX-FileCopyrightText: AnAkkk <anakin.cs@gmail.com>
# SPDX-FileCopyrightText: Andrea Scarpino <andrea@archlinux.org>
# SPDX-FileCopyrightText: David Edmundson <kde@davidedmundson.co.uk>
# SPDX-FileCopyrightText: Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: Jerome Leclanche <jerome@leclan.ch>
# SPDX-FileCopyrightText: Kurt Kremitzki <kkremitzki@posteo.net>
# SPDX-FileCopyrightText: Martin Briza <mbriza@redhat.com>
# SPDX-FileCopyrightText: Neal Gompa <ngompa@kde.org>
# SPDX-FileCopyrightText: Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-FileCopyrightText: Nikita Mikhailov <nslqqq@gmail.com>
# SPDX-FileCopyrightText: Nikita Mikhaylov <nslqqq@gmail.com>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
install(FILES
    "org.freedesktop.DisplayManager.conf"
    DESTINATION "${DBUS_CONFIG_DIR}"
    RENAME ${DBUS_CONFIG_FILENAME}
)

install(FILES
    "scripts/Xsession"
    "scripts/Xsetup"
    "scripts/Xstop"
    "scripts/wayland-session"
    DESTINATION "${DATA_INSTALL_DIR}/scripts"
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

if(INSTALL_PAM_CONFIGURATION)
    set(PAM_OS_CONFIGURATION "auto"
        CACHE STRING "PAM configuration data for operating system. Valid options: auto;fedora;suse"
    )
    if(PAM_OS_CONFIGURATION STREQUAL "auto")
        set(PAM_OS_CONFIGURATION "" CACHE INTERNAL "No detected system" FORCE)

        # Get os-release(5) ID data
        cmake_host_system_information(RESULT OS_ID QUERY DISTRIB_ID)
        cmake_host_system_information(RESULT OS_ID_LIKE QUERY DISTRIB_ID_LIKE)
        if(NOT "" STREQUAL OS_ID_LIKE)
            string(REPLACE " " ";" OS_ID_LIST ${OS_ID_LIKE})
        endif()
        list(APPEND OS_ID_LIST "${OS_ID}")
        list(REMOVE_DUPLICATES OS_ID_LIST)
        # Check if OS matches for available configuration
        if("fedora" STREQUAL OS_ID OR "fedora" IN_LIST OS_ID_LIST)
           message(STATUS "Installing PAM configuration for Fedora Linux-like systems.")
           set(PAM_OS_CONFIGURATION "fedora" CACHE INTERNAL "Auto-detected Fedora Linux type system" FORCE)
        elseif("suse" IN_LIST OS_ID_LIST)
           message(STATUS "Installing PAM configuration for SUSE Linux-like systems.")
           set(PAM_OS_CONFIGURATION "suse" CACHE INTERNAL "Auto-detected SUSE Linux type system" FORCE)
        elseif("kde-linux" IN_LIST OS_ID_LIST)
           message(STATUS "Installing PAM configuration for KDE Linux-like systems.")
           set(PAM_OS_CONFIGURATION "kde-linux" CACHE INTERNAL "Auto-detected KDE Linux type system" FORCE)
        elseif("arch" IN_LIST OS_ID_LIST)
           message(STATUS "Installing PAM configuration for Arch Linux-like systems.")
           set(PAM_OS_CONFIGURATION "arch" CACHE INTERNAL "Auto-detected Arch Linux type system" FORCE)
        else()
           message(FATAL_ERROR
               "Unknown operating system detected!\n"
               "No provided PAM configuration files exist for OS ID '${OS_ID}' or its ID_LIKE '${OS_ID_LIKE}'.\n"
               "Please re-run with '-DINSTALL_PAM_CONFIGURATION:BOOL=OFF' and provide your own PAM configuration."
           )
        endif()
    endif()

    # Install pam files
    install(FILES pam/${PAM_OS_CONFIGURATION}/plasmalogin-autologin DESTINATION ${PAM_CONFIG_DIR})
    install(FILES pam/${PAM_OS_CONFIGURATION}/plasmalogin-greeter DESTINATION ${PAM_CONFIG_DIR})
    install(FILES pam/${PAM_OS_CONFIGURATION}/plasmalogin DESTINATION ${PAM_CONFIG_DIR})
endif()
