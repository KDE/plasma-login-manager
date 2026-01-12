## SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: David Edmundson <kde@davidedmundson.co.uk>
# SPDX-FileCopyrightText: Oliver Beard <olib141@outlook.com>
add_executable(plasma-login-wallpaper)

ecm_add_qml_module(plasma-login-wallpaper URI "org.kde.plasma.login.wallpaper" GENERATE_PLUGIN_SOURCE)

target_sources(plasma-login-wallpaper PRIVATE
    main.cpp
    wallpaperapp.cpp
    wallpaperwindow.cpp
)

ecm_target_qml_sources(plasma-login-wallpaper PRIVATE
    SOURCES
    main.qml
    WallpaperFader.qml
)

target_link_libraries(plasma-login-wallpaper PRIVATE
    settings
    Qt::Core
    Qt::Gui
    Qt::DBus
    Qt::Quick
    KF6::WindowSystem
    KF6::Package
    Plasma::PlasmaQuick
    LayerShellQt::Interface
)

qt_add_shaders(plasma-login-wallpaper "wallpaper_shaders"
    PRECOMPILE
    BATCHABLE
    OPTIMIZED
    PREFIX "/qt/qml/org/kde/plasma/login/wallpaper/shaders"
    FILES
        WallpaperFader.frag
    OUTPUTS
        WallpaperFader.frag.qsb
)

ecm_finalize_qml_module(plasma-login-wallpaper)

install(TARGETS plasma-login-wallpaper DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
ecm_install_configured_files(INPUT plasma-wallpaper.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
