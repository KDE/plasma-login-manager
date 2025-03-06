add_subdirectory(settings)

add_executable(plasma-login-wallpaper)

target_sources(plasma-login-wallpaper PRIVATE
    main.cpp
    wallpaperapp.cpp
    wallpaperwindow.cpp
    settings/wallpaper_integration.cpp
)

target_link_libraries(plasma-login-wallpaper PRIVATE
    settings
    Qt::Core
    Qt::Gui
    Qt::Quick
    KF6::WindowSystem
    KF6::Package
    Plasma::PlasmaQuick
    LayerShellQt::Interface
)

install(TARGETS plasma-login-wallpaper DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
