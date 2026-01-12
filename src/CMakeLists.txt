## SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Abdurrahman AVCI <abdurrahmanavci@gmail.com>
# SPDX-FileCopyrightText: David Edmundson <kde@davidedmundson.co.uk>
# SPDX-FileCopyrightText: Jerome Leclanche <jerome@leclan.ch>
# SPDX-FileCopyrightText: Martin Briza <mbriza@redhat.com>
# SPDX-FileCopyrightText: Nikita Mikhailov <nslqqq@gmail.com>
# SPDX-FileCopyrightText: Nikita Mikhaylov <nslqqq@gmail.com>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
configure_file("common/Constants.h.in" "common/Constants.h" IMMEDIATE @ONLY)

add_subdirectory(daemon)
add_subdirectory(helper)
add_subdirectory(frontend)
