include_directories(
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
    "${CMAKE_BINARY_DIR}/src/common"
    ${LIBXAU_INCLUDE_DIRS}
)

configure_file(config.h.in config.h IMMEDIATE @ONLY)
set(DAEMON_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/Configuration.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SafeDataStream.cpp
    ${CMAKE_SOURCE_DIR}/src/common/ConfigReader.cpp
    ${CMAKE_SOURCE_DIR}/src/common/Session.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SocketWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/common/XAuth.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SignalHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/Auth.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthPrompt.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthRequest.cpp

    DaemonApp.cpp
    Display.cpp
    DisplayManager.cpp
    DisplayServer.cpp
    LogindDBusTypes.cpp
    Greeter.cpp
    Seat.cpp
    SeatManager.cpp
    SocketServer.cpp
    XorgDisplayServer.cpp
    XorgUserDisplayServer.cpp
    XorgUserDisplayServer.h
    WaylandDisplayServer.cpp
    WaylandDisplayServer.h
)

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
                      Qt${QT_MAJOR_VERSION}::DBus
                      Qt${QT_MAJOR_VERSION}::Network
                      Qt${QT_MAJOR_VERSION}::Qml
                      ${LIBXAU_LINK_LIBRARIES}
                      ${PAM_LIBRARIES})

if(JOURNALD_FOUND)
    target_link_libraries(plasmalogin ${JOURNALD_LIBRARIES})
endif()

install(TARGETS plasmalogin DESTINATION "${CMAKE_INSTALL_BINDIR}")
