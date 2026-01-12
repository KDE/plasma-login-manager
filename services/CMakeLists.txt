## SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Aaron Seigo <aseigo@kde.org>
# SPDX-FileCopyrightText: Abdurrahman AVCI <abdurrahmanavci@gmail.com>
# SPDX-FileCopyrightText: Aleix Pol <aleixpol@kde.org>
# SPDX-FileCopyrightText: carlosdem <60242467+carlosdem@users.noreply.github.com>
# SPDX-FileCopyrightText: Carlos De Maine <carlosdemaine@gmail.com>
# SPDX-FileCopyrightText: David Edmundson <kde@davidedmundson.co.uk>
# SPDX-FileCopyrightText: Emil Velikov <emil.l.velikov@gmail.com>
# SPDX-FileCopyrightText: Fabian Vogt <fabian@ritter-vogt.de>
# SPDX-FileCopyrightText: Gleb Popov <6yearold@gmail.com>
# SPDX-FileCopyrightText: Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: Jerome Leclanche <jerome@leclan.ch>
# SPDX-FileCopyrightText: Kristen McWilliam <kristen@kde.org>
# SPDX-FileCopyrightText: Neal Gompa <neal@gompa.dev>
# SPDX-FileCopyrightText: Oliver Beard <olib141@outlook.com>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@liri.io>
if(SYSTEMD_FOUND)
    configure_file(plasmalogin.service.in plasmalogin.service)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/plasmalogin.service"
        DESTINATION "${KDE_INSTALL_SYSTEMDUNITDIR}/system"
    )

    configure_file(plasmalogin-sysuser.conf.in plasmalogin-sysuser.conf)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/plasmalogin-sysuser.conf"
        DESTINATION "lib/sysusers.d"
        RENAME plasmalogin.conf
    )
endif()

# systemd-tmpfiles can be used standalone without other systemd parts
if(DEFINED SYSTEMD_TMPFILES_DIR)
    configure_file(plasmalogin-tmpfiles.conf.in plasmalogin-tmpfiles.conf)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/plasmalogin-tmpfiles.conf"
        DESTINATION "lib/tmpfiles.d"
        RENAME plasmalogin.conf
    )
endif()
