cmake_minimum_required(VERSION 3.16)

project(plasma-login)
set(PROJECT_VERSION "6.3.0")

set(PROJECT_DEP_VERSION "6.3.0")
set(QT_MIN_VERSION "6.7.0")
set(KF6_MIN_VERSION "6.10.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

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


find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Quick)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config Package WindowSystem I18n)
find_package(PlasmaQuick ${PROJECT_DEP_VERSION} REQUIRED)
find_package(LayerShellQt ${PROJECT_DEP_VERSION} REQUIRED)

add_subdirectory(wallpaper)
add_subdirectory(prompt)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
