# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>

kauth_install_actions(org.kde.kcontrol.kcmplasmalogin kcm_plasmalogin.actions)

add_executable(kcmplasmalogin_authhelper plasmaloginauthhelper.cpp plasmaloginauthhelper.h)
target_link_libraries(kcmplasmalogin_authhelper settings KF6::AuthCore KF6::ConfigCore KF6::I18n Qt6::DBus)

kauth_install_helper_files(kcmplasmalogin_authhelper org.kde.kcontrol.kcmplasmalogin root)
install(TARGETS kcmplasmalogin_authhelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
