install(FILES plasma-login-wayland.target DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

add_executable(startplasma-login-wayland startplasmalogin-wayland.cpp)
target_link_libraries(startplasma-login-wayland PRIVATE
    Qt::Core
    Qt::DBus
    KF6::ConfigCore
    # KF6::DBusAddons
    # KF6::Notifications
)
install(TARGETS startplasma-login-wayland ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
