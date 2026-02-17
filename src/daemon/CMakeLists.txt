include_directories(
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
    "${CMAKE_BINARY_DIR}/src/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/backend"
)

configure_file(config.h.in config.h IMMEDIATE @ONLY)
set(DAEMON_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/Configuration.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SafeDataStream.cpp
    ${CMAKE_SOURCE_DIR}/src/common/ConfigReader.cpp
    ${CMAKE_SOURCE_DIR}/src/common/Session.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SocketWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/common/VirtualTerminal.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/Auth.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthPrompt.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthRequest.cpp

    DaemonApp.cpp
    Display.cpp
    DisplayManager.cpp
    Greeter.cpp
    Seat.cpp
    SeatManager.cpp
    SocketServer.cpp

    # Backend abstraction
    backend/SessionBackend.cpp
    backend/FallbackBackend.cpp
)

qt_add_dbus_adaptor(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.DisplayManager.xml"          "DisplayManager.h" PLASMALOGIN::DisplayManager)
qt_add_dbus_adaptor(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.DisplayManager.Seat.xml"     "DisplayManager.h" PLASMALOGIN::DisplayManagerSeat)
qt_add_dbus_adaptor(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.DisplayManager.Session.xml"  "DisplayManager.h" PLASMALOGIN::DisplayManagerSession)

# Logind/elogind backend (uses same DBus interface)
if(HAVE_SYSTEMD OR HAVE_ELOGIND)
    list(APPEND DAEMON_SOURCES
        LogindDBusTypes.cpp
        backend/LogindBackend.cpp
    )

    set_source_files_properties("${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.login1.Manager.xml" PROPERTIES
       INCLUDE "LogindDBusTypes.h"
    )
    set_source_files_properties("${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.login1.Seat.xml" PROPERTIES
       INCLUDE "LogindDBusTypes.h"
    )
    set_source_files_properties("${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.login1.Session.xml" PROPERTIES
       INCLUDE "LogindDBusTypes.h"
    )

    qt_add_dbus_interface(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.login1.Manager.xml"  "Login1Manager")
    qt_add_dbus_interface(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.login1.Seat.xml"  "Login1Seat")
    qt_add_dbus_interface(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.login1.Session.xml"  "Login1Session")
endif()

# ConsoleKit2 backend
if(HAVE_CONSOLEKIT2)
    list(APPEND DAEMON_SOURCES
        backend/ConsoleKit2Backend.cpp
    )

    qt_add_dbus_interface(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.ConsoleKit.Manager.xml"  "ConsoleKitManager")
    qt_add_dbus_interface(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.ConsoleKit.Seat.xml"  "ConsoleKitSeat")
    qt_add_dbus_interface(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.ConsoleKit.Session.xml"  "ConsoleKitSession")
endif()

add_executable(plasmalogin ${DAEMON_SOURCES})

# Compile definitions for backend availability
if(HAVE_SYSTEMD)
    target_compile_definitions(plasmalogin PRIVATE HAVE_SYSTEMD)
endif()
if(HAVE_ELOGIND)
    target_compile_definitions(plasmalogin PRIVATE HAVE_ELOGIND)
endif()
if(HAVE_CONSOLEKIT2)
    target_compile_definitions(plasmalogin PRIVATE HAVE_CONSOLEKIT2)
endif()

# Base libraries
target_link_libraries(plasmalogin
                      Qt6::DBus
                      Qt6::Network
                      Qt6::Qml
                      ${PAM_LIBRARIES}
                      KF6::CoreAddons
                      KF6::ConfigCore
                      KF6::ConfigGui)

# Conditional backend linking
if(HAVE_SYSTEMD)
    target_link_libraries(plasmalogin PkgConfig::LIBSYSTEMD)
elseif(HAVE_ELOGIND)
    target_link_libraries(plasmalogin PkgConfig::LIBELOGIND)
endif()

if(HAVE_CONSOLEKIT2)
    target_link_libraries(plasmalogin PkgConfig::CONSOLEKIT2)
endif()

install(TARGETS plasmalogin DESTINATION "${CMAKE_INSTALL_BINDIR}")
