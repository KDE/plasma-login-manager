install(FILES plasma-login-wayland.target DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
ecm_install_configured_files(INPUT plasma-login-kwin_wayland.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

ecm_qt_declare_logging_category(startplasma_SRCS HEADER debug.h IDENTIFIER PLASMA_STARTUP CATEGORY_NAME org.kde.startup)

add_executable(startplasma-login-wayland startplasmalogin-wayland.cpp startplasma.cpp ${startplasma_SRCS})
target_link_libraries(startplasma-login-wayland PRIVATE
    Qt::Core
    Qt::DBus
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::DBusAddons
    KF6::Package
    PW::KLookAndFeel
)

kconfig_target_kcfg_file(startplasma-login-wayland
    FILE lookandfeelsettings.kcfg
    CLASS_NAME LookAndFeelSettings
)

install(TARGETS startplasma-login-wayland ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
