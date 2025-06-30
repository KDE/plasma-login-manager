cmake_minimum_required(VERSION 3.16)

project(PLASMALOGIN)

# Set version
set(PLASMALOGIN_VERSION_MAJOR 0)
set(PLASMALOGIN_VERSION_MINOR 21)
set(PLASMALOGIN_VERSION_PATCH 0)
set(PLASMALOGIN_VERSION_STRING "${PLASMALOGIN_VERSION_MAJOR}.${PLASMALOGIN_VERSION_MINOR}.${PLASMALOGIN_VERSION_PATCH}")
add_compile_definitions("PLASMALOGIN_VERSION=\"${PLASMALOGIN_VERSION_STRING}\"")

# Options
option(BUILD_MAN_PAGES "Build man pages" OFF)
option(ENABLE_JOURNALD "Enable logging to journald" ON)
option(NO_SYSTEMD "Disable systemd support" OFF)
option(USE_ELOGIND "Use elogind instead of logind" OFF)

# Version settings
set(PROJECT_DEP_VERSION "6.3.0")
set(QT_MIN_VERSION "6.7.0")
set(KF6_MIN_VERSION "6.10.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Include KDE CMake modules
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMMarkNonGuiExecutable)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(GenerateExportHeader)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)
include(ECMQtDeclareLoggingCategory)
include(ECMConfiguredInstall)
include(ECMQmlModule)

find_package(Qt6 CONFIG REQUIRED Core DBus Gui Qml Quick LinguistTools Test QuickTest)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config Package WindowSystem I18n DBusAddons KCMUtils Auth KIO)
find_package(PlasmaQuick ${PROJECT_DEP_VERSION} REQUIRED)
find_package(LayerShellQt ${PROJECT_DEP_VERSION} REQUIRED)
find_package(LibKWorkspace ${PROJECT_DEP_VERSION} REQUIRED)

# Standard installation paths
include(GNUInstallDirs)

# PAM
find_package(PAM REQUIRED)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)

# Macros
include(FeatureSummary)

# PKG-CONFIG
find_package(PkgConfig)

# XAU
pkg_check_modules(LIBXAU REQUIRED "xau")

# XCB
find_package(XCB MODULE REQUIRED COMPONENTS AUX XCB CURSOR RANDR IMAGE)

# # XKB
# find_package(XKB REQUIRED)




# systemd
if(NOT NO_SYSTEMD AND NOT USE_ELOGIND)
    pkg_check_modules(SYSTEMD "systemd")
endif()

if(SYSTEMD_FOUND)
    add_definitions(-DHAVE_SYSTEMD)
    set(CMAKE_AUTOMOC_MOC_OPTIONS -DHAVE_SYSTEMD)

    # libsystemd-journal was merged into libsystemd in 209
    if(${SYSTEMD_VERSION} VERSION_LESS 209)
        pkg_check_modules(JOURNALD "libsystemd-journal")
    else()
        pkg_check_modules(JOURNALD "libsystemd")
    endif()

    if(ENABLE_JOURNALD)
        if(JOURNALD_FOUND)
            add_definitions(-DHAVE_JOURNALD)
            set(CMAKE_AUTOMOC_MOC_OPTIONS -DHAVE_JOURNALD)
        else()
            message(WARNING "Disable journald support for lack of libsystemd-journal")
        endif()
    endif()

    if (NOT DEFINED SYSTEMD_SYSTEM_UNIT_DIR)
        pkg_get_variable(SYSTEMD_SYSTEM_UNIT_DIR systemd systemdsystemunitdir)
    endif()

    if (NOT DEFINED SYSTEMD_SYSUSERS_DIR)
        pkg_get_variable(SYSTEMD_SYSUSERS_DIR systemd sysusersdir)
    endif()

    if (NOT DEFINED SYSTEMD_TMPFILES_DIR)
        pkg_get_variable(SYSTEMD_TMPFILES_DIR systemd tmpfilesdir)
    endif()

else()
    set(SYSTEMD_FOUND 0)
endif()
add_feature_info("systemd" SYSTEMD_FOUND "systemd support")
add_feature_info("journald" JOURNALD_FOUND "journald support")

# elogind
if(NO_SYSTEMD AND USE_ELOGIND)
    pkg_check_modules(ELOGIND "libelogind")
endif()


if (SYSTEMD_FOUND)
    set(RUNTIME_DIR_DEFAULT "/run/plasmalogin")
else()
    set(RUNTIME_DIR_DEFAULT "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/run/plasmalogin")
endif()

# utmps
find_package(utmps)
add_feature_info("utmps" UTMPS_FOUND "utmps support")

# Set constants
set(DATA_INSTALL_DIR            "${CMAKE_INSTALL_FULL_DATADIR}/plasmalogin"                CACHE PATH      "System application data install directory")
set(DBUS_CONFIG_DIR             "${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/system.d"       CACHE PATH      "DBus config files directory")
set(STATE_DIR                   "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/plasmalogin"      CACHE PATH      "State directory")
set(RUNTIME_DIR                 "${RUNTIME_DIR_DEFAULT}"                            CACHE PATH      "Runtime data storage directory")

set(SESSION_COMMAND             "${DATA_INSTALL_DIR}/scripts/Xsession"              CACHE PATH      "Script to execute when starting the X11 desktop session")
set(WAYLAND_SESSION_COMMAND     "${DATA_INSTALL_DIR}/scripts/wayland-session"       CACHE PATH      "Script to execute when starting the Wayland desktop session")

set(CONFIG_FILE                 "${CMAKE_INSTALL_FULL_SYSCONFDIR}/plasmalogin.conf"        CACHE PATH      "Path of the plasmalogin config file")
set(CONFIG_DIR                  "${CMAKE_INSTALL_FULL_SYSCONFDIR}/plasmalogin.conf.d"      CACHE PATH      "Path of the plasmalogin config directory")
set(SYSTEM_CONFIG_DIR           "${CMAKE_INSTALL_PREFIX}/lib/plasmalogin/plasmalogin.conf.d"      CACHE PATH      "Path of the system plasmalogin config directory")
set(LOG_FILE                    "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/plasmalogin.log"  CACHE PATH      "Path of the plasmalogin log file")
set(DBUS_CONFIG_FILENAME        "org.freedesktop.DisplayManager.conf"               CACHE STRING    "Name of the plasmalogin config file")
set(PLASMALOGIN_INITIAL_VT             "1"                                                 CACHE STRING    "Initial tty to use")


# Autodetect UID_MIN and UID_MAX from /etc/login.defs
if(NOT DEFINED LOGIN_DEFS_PATH)
    set(LOGIN_DEFS_PATH "/etc/login.defs" CACHE PATH "Path to login.defs")
endif()

if(NOT EXISTS "${LOGIN_DEFS_PATH}" AND NOT DEFINED UID_MIN AND NOT DEFINED UID_MAX)
    message(FATAL_ERROR "Could not find login.defs at ${LOGIN_DEFS_PATH}. Try -DLOGIN_DEFS_PATH.")
endif()

if(NOT DEFINED UID_MIN)
    file(STRINGS "${LOGIN_DEFS_PATH}" UID_MIN LIMIT_COUNT 1 REGEX "^UID_MIN")
    string(REGEX REPLACE "^UID_MIN[\t ]+([0-9]+)" "\\1" UID_MIN "${UID_MIN}")
    if(NOT UID_MIN MATCHES "[0-9]+")
        message(WARNING "Could not detect a valid UID_MIN (got '${UID_MIN}'). You should review your login.defs file.")
        set(UID_MIN 1000)
    else()
        message(STATUS "Autodetected UID_MIN=${UID_MIN} from ${LOGIN_DEFS_PATH}")
    endif()
endif()

if(NOT DEFINED UID_MAX)
    file(STRINGS "${LOGIN_DEFS_PATH}" UID_MAX LIMIT_COUNT 1 REGEX "^UID_MAX")
    string(REGEX REPLACE "^UID_MAX[\t ]+([0-9]+)" "\\1" UID_MAX "${UID_MAX}")
    if(NOT UID_MAX MATCHES "[0-9]+")
        message(WARNING "Could not detect a valid UID_MAX (got '${UID_MAX}'). You should review your login.defs file.")
        set(UID_MAX 65000)
    else()
        message(STATUS "Autodetected UID_MAX=${UID_MAX} from ${LOGIN_DEFS_PATH}")
    endif()
endif()

# Add subdirectories
add_subdirectory(data)
add_subdirectory(services)
add_subdirectory(src)

add_subdirectory(test)

# Display feature summary
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
