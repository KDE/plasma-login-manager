include(CheckLibraryExists)

include_directories(
    "${CMAKE_SOURCE_DIR}/src/auth"
)

add_executable(plasmalogin-helper)

target_sources(plasmalogin-helper PRIVATE
    ${CMAKE_SOURCE_DIR}/src/auth/Auth.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthRequest.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthPrompt.cpp
    HelperApp.cpp
    UserSession.cpp
    backend/PamHandle.cpp
    backend/PamBackend.cpp
)

target_link_libraries(plasmalogin-helper
                      plasmalogin-common
                      Qt6::Network
                      Qt6::DBus
                      Qt6::Qml
                      KF6::CoreAddons
                      KF6::ConfigGui
                      PkgConfig::LIBSYSTEMD)

target_link_libraries(plasmalogin-helper ${PAM_LIBRARIES})

install(TARGETS plasmalogin-helper RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")

add_executable(plasmalogin-helper-start-x11user HelperStartX11User.cpp xorguserhelper.cpp XAuth.cpp)

target_link_libraries(plasmalogin-helper-start-x11user Qt6::Core KF6::CoreAddons PkgConfig::LIBXAU PkgConfig::LIBSYSTEMD)

target_link_libraries(plasmalogin-helper-start-x11user
                      Qt6::Core
                      PkgConfig::LIBXAU
                      PkgConfig::LIBSYSTEMD
                      plasmalogin-common)

install(TARGETS plasmalogin-helper-start-x11user RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
