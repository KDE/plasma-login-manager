## SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Andreas Cord-Landwehr <cordlandwehr@kde.org>
# SPDX-FileCopyrightText: David Edmundson <kde@davidedmundson.co.uk>
# SPDX-FileCopyrightText: Fabian Vogt <fabian@ritter-vogt.de>
# SPDX-FileCopyrightText: Fabian Vogt <fvogt@suse.de>
# SPDX-FileCopyrightText: Martin Briza <mbriza@redhat.com>
# SPDX-FileCopyrightText: Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-FileCopyrightText: Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
set(QT_USE_QTTEST TRUE)

include_directories(../src/common)

set(ConfigurationTest_SRCS ConfigurationTest.cpp ../src/common/ConfigReader.cpp)
add_executable(ConfigurationTest ${ConfigurationTest_SRCS})
add_test(NAME Configuration COMMAND ConfigurationTest)

target_link_libraries(ConfigurationTest Qt6::Core Qt6::Test)

# set(SessionTest_SRCS SessionTest.cpp ../src/common/Configuration.cpp ../src/common/ConfigReader.cpp ../src/common/Session.cpp)
# add_executable(SessionTest ${SessionTest_SRCS})
# target_include_directories(SessionTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../src/common)
# add_test(NAME Session COMMAND SessionTest)
# target_link_libraries(SessionTest Qt6::Core Qt6::Test)
