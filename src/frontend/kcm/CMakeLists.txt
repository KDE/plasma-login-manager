set(PLASMALOGIN_CONFIG_FILE         "/etc/plasma-login.conf"                                        CACHE PATH  "Path of the plasma-login config file")
set(PLASMALOGIN_CONFIG_DIR          "/etc/plasma-login.conf.d"                                      CACHE PATH  "Path of the plasma-login config directory")
set(PLASMALOGIN_SYSTEM_CONFIG_DIR   "${CMAKE_INSTALL_PREFIX}/lib/plasma-login/plasma-login.conf.d"  CACHE PATH  "Path of the system plasma-login config directory")
set(XSESSIONS_DIR                   "${CMAKE_INSTALL_PREFIX}/share/xsessions"                       CACHE PATH  "Path of the xsessions") # TODO: Remove, use new SessionModel
set(WAYLAND_SESSIONS_DIR            "${CMAKE_INSTALL_PREFIX}/share/wayland-sessions"                CACHE PATH  "Path of the wayland sessions") # TODO: Remove, use new SessionModel

configure_file(config.h.in config.h IMMEDIATE @ONLY)

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_plasmalogin\")

kcmutils_add_qml_kcm(kcm_plasmalogin)

target_sources(kcm_plasmalogin PRIVATE
    kcm.cpp kcm.h
    models/sessionmodel.cpp models/sessionmodel.h
    models/usersmodel.cpp models/usersmodel.h
    plasmalogindata.cpp plasmalogindata.h
    plasmaloginsettingsbase.cpp plasmaloginsettingsbase.h
)

kconfig_add_kcfg_files(kcm_plasmalogin plasmaloginsettings.kcfgc GENERATE_MOC)

target_link_libraries(kcm_plasmalogin PRIVATE
    KF6::I18n
    KF6::AuthCore
    KF6::KCMUtils
    KF6::KIOGui
    KF6::Service
)

add_subdirectory(auth)
