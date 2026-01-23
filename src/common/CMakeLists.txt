configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Constants.h.in ${CMAKE_CURRENT_BINARY_DIR}/Constants.h IMMEDIATE @ONLY)

add_library(plasmalogin-common OBJECT
    SafeDataStream.cpp
    Session.cpp
    SocketWriter.cpp
    VirtualTerminal.cpp
    MainConfigLoader.cpp
)

# Generate KConfigXT sources for the shared main configuration
kconfig_add_kcfg_files(plasmalogin-common GENERATE_MOC
    ${CMAKE_CURRENT_SOURCE_DIR}/mainconfig.kcfgc)

target_include_directories(plasmalogin-common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(plasmalogin-common
    PUBLIC
        Qt6::Core
        Qt6::Network
        KF6::ConfigGui
)
