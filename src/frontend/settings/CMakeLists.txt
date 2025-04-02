set(PLASMALOGIN_CONFIG_FILE         "/etc/plasma-login.conf"                                    CACHE PATH  "Path of the plasma-login config file")
set(PLASMALOGIN_SYSTEM_CONFIG_FILE  "${CMAKE_INSTALL_PREFIX}/lib/plasma-login/defaults.conf"    CACHE PATH  "Path of the system plasma-login config file")
set(XSESSIONS_DIR                   "${CMAKE_INSTALL_PREFIX}/share/xsessions"                       CACHE PATH  "Path of the xsessions") # TODO: Remove, use new SessionModel
set(WAYLAND_SESSIONS_DIR            "${CMAKE_INSTALL_PREFIX}/share/wayland-sessions"                CACHE PATH  "Path of the wayland sessions") # TODO: Remove, use new SessionModel

configure_file(config.h.in config.h IMMEDIATE @ONLY)

add_library(settings STATIC
    config.h
    plasmaloginsettings.cpp
    plasmaloginsettingsdefaults.cpp
#    shell_integration.cpp
    wallpaperintegration.cpp
    wallpapersettings.cpp
    models/sessionmodel.cpp models/sessionmodel.h
    models/usermodel.cpp models/usermodel.h
)
set_property(TARGET settings PROPERTY POSITION_INDEPENDENT_CODE ON)

kconfig_add_kcfg_files(settings GENERATE_MOC plasmaloginsettingsbase.kcfgc)
target_link_libraries(settings
    Qt6::Quick
    KF6::ConfigQml
#    KF6::GlobalAccel
    KF6::I18n
    KF6::Package
#    KF6::XmlGui
)
