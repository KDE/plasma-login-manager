## SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Adriaan de Groot <groot@kde.org>
# SPDX-FileCopyrightText: Aleix Pol <aleixpol@kde.org>
# SPDX-FileCopyrightText: Andreas Cord-Landwehr <cordlandwehr@kde.org>
# SPDX-FileCopyrightText: David Edmundson <kde@davidedmundson.co.uk>
# SPDX-FileCopyrightText: Dāvis Mosāns <davispuh@gmail.com>
# SPDX-FileCopyrightText: Fabian Vogt <fabian@ritter-vogt.de>
# SPDX-FileCopyrightText: Gleb Popov <6yearold@gmail.com>
# SPDX-FileCopyrightText: Marius Bakke <mbakke@fastmail.com>
# SPDX-FileCopyrightText: Martin Briza <mbriza@redhat.com>
# SPDX-FileCopyrightText: Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-FileCopyrightText: Peter <petershh@disroot.org>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@liri.io>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <plfiorini@users.noreply.github.com>
# SPDX-FileCopyrightText: Raphael Kubo da Costa <rakuco@FreeBSD.org>
include(CheckLibraryExists)

include_directories(
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
)
include_directories("${CMAKE_BINARY_DIR}/src/common")

add_executable(plasmalogin-helper)

target_sources(plasmalogin-helper PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common/Configuration.cpp
    ${CMAKE_SOURCE_DIR}/src/common/ConfigReader.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SafeDataStream.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SignalHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/Auth.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthRequest.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthPrompt.cpp
    ${CMAKE_SOURCE_DIR}/src/common/VirtualTerminal.cpp
    HelperApp.cpp
    UserSession.cpp
    backend/PamHandle.cpp
    backend/PamBackend.cpp
)

target_link_libraries(plasmalogin-helper
                      Qt6::Network
                      Qt6::DBus
                      Qt6::Qml
                      PkgConfig::LIBSYSTEMD)

target_link_libraries(plasmalogin-helper ${PAM_LIBRARIES})

install(TARGETS plasmalogin-helper RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")

add_executable(plasmalogin-helper-start-x11user HelperStartX11User.cpp xorguserhelper.cpp XAuth.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/ConfigReader.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/Configuration.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/SignalHandler.cpp
                                                )
target_link_libraries(plasmalogin-helper-start-x11user Qt6::Core PkgConfig::LIBXAU PkgConfig::LIBSYSTEMD)
install(TARGETS plasmalogin-helper-start-x11user RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
