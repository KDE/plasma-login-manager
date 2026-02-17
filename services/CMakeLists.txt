# systemd service and related files
if(HAVE_SYSTEMD AND SYSTEMD_FOUND)
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

# OpenRC init script (Gentoo, Alpine, etc.)
option(WITH_OPENRC "Install OpenRC init script" OFF)
if(WITH_OPENRC)
    configure_file(plasmalogin.openrc.in plasmalogin)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/plasmalogin"
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/init.d"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()

# FreeBSD rc.d script
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    configure_file(plasmalogin.rc.in plasmalogin)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/plasmalogin"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/etc/rc.d"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()
