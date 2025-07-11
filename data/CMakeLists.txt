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
