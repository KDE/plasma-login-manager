# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

find_package(Corrosion REQUIRED)
corrosion_import_crate(MANIFEST_PATH Cargo.toml)
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/plasma-login-failure DESTINATION "${KDE_INSTALL_LIBEXECDIR}")

ecm_install_configured_files(INPUT plasma-login-failure.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/system/)
