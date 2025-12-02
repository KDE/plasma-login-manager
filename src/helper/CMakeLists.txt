include(CheckLibraryExists)

include_directories(
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
    ${LIBXAU_INCLUDE_DIRS}
)
include_directories("${CMAKE_BINARY_DIR}/src/common")

set(HELPER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/Configuration.cpp
    ${CMAKE_SOURCE_DIR}/src/common/ConfigReader.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SafeDataStream.cpp
    ${CMAKE_SOURCE_DIR}/src/common/XAuth.cpp
    ${CMAKE_SOURCE_DIR}/src/common/SignalHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/Auth.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthRequest.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthPrompt.cpp
    Backend.cpp
    HelperApp.cpp
    UserSession.cpp
)

list(APPEND HELPER_SOURCES ${CMAKE_SOURCE_DIR}/src/common/VirtualTerminal.cpp)

set(HELPER_SOURCES
    ${HELPER_SOURCES}
    backend/PamHandle.cpp
    backend/PamBackend.cpp
)

add_executable(plasmalogin-helper ${HELPER_SOURCES})
target_link_libraries(plasmalogin-helper
                      Qt6::Network
                      Qt6::DBus
                      Qt6::Qml
                      PkgConfig::LIBSYSTEMD
                      ${LIBXAU_LINK_LIBRARIES})
if("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
    # On FreeBSD (possibly other BSDs as well), we want to use
    # setusercontext() to set up the login configuration from login.conf
    find_library(_have_libutil util)
    if(_have_libutil)
        check_library_exists(${_have_libutil} setusercontext "" _have_setusercontext)
    endif()
    if(_have_libutil AND _have_setusercontext)
        target_link_libraries(plasmalogin-helper ${_have_libutil})
    endif()
endif()

target_link_libraries(plasmalogin-helper ${PAM_LIBRARIES})

install(TARGETS plasmalogin-helper RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")

add_executable(plasmalogin-helper-start-x11user HelperStartX11User.cpp xorguserhelper.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/ConfigReader.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/Configuration.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/XAuth.cpp
                                                ${CMAKE_SOURCE_DIR}/src/common/SignalHandler.cpp
                                                )
target_link_libraries(plasmalogin-helper-start-x11user Qt6::Core
                                                ${LIBXAU_LINK_LIBRARIES} PkgConfig::LIBSYSTEMD)
install(TARGETS plasmalogin-helper-start-x11user RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
