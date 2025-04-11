#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` -o $podir/kcm_plasma_login.pot
