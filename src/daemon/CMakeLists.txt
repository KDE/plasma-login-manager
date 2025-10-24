include_directories(
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
    "${CMAKE_BINARY_DIR}/src/common"
)

configure_file(config.h.in config.h IMMEDIATE @ONLY)

set(SYSTEMD_DBUS_XMLS
      org.freedesktop.systemd1.Manager.xml)
set_source_files_properties(${SYSTEMD_DBUS_XMLS} PROPERTIES INCLUDE dbustypes.h)

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
    LogindDBusTypes.cpp
    Greeter.cpp
    Seat.cpp
    SeatManager.cpp
    SessionRunner.cpp
    SocketServer.cpp
    XorgUserDisplayServer.cpp
)

qt_add_dbus_interfaces(DAEMON_SOURCES ${SYSTEMD_DBUS_XMLS})


list(APPEND DAEMON_SOURCES ${CMAKE_SOURCE_DIR}/src/common/VirtualTerminal.cpp)

qt_add_dbus_adaptor(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.DisplayManager.xml"          "DisplayManager.h" PLASMALOGIN::DisplayManager)
qt_add_dbus_adaptor(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.DisplayManager.Seat.xml"     "DisplayManager.h" PLASMALOGIN::DisplayManagerSeat)
qt_add_dbus_adaptor(DAEMON_SOURCES "${CMAKE_SOURCE_DIR}/data/interfaces/org.freedesktop.DisplayManager.Session.xml"  "DisplayManager.h" PLASMALOGIN::DisplayManagerSession)

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

add_executable(plasmalogin ${DAEMON_SOURCES})
target_link_libraries(plasmalogin
                      Qt6::DBus
                      Qt6::Network
                      Qt6::Qml
                      ${PAM_LIBRARIES}
                      KF6::CoreAddons
                      KF6::ConfigCore
                      KF6::ConfigGui
                      PkgConfig::LIBSYSTEMD)

install(TARGETS plasmalogin DESTINATION "${CMAKE_INSTALL_BINDIR}")
