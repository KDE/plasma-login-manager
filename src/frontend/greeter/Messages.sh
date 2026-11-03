#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
# SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

# Ensure passing shellcheck:
# Our l10n scripting isn't working with spaces anywhere and we actively rely on word splitting in our Messages.sh.
# shellcheck disable=SC2046

set -eu

podir=${podir:?} # ensure it is defined

$XGETTEXT `find . -name "*.cpp" -o -name "*.qml"` `find ../settings/ -name "*.cpp"` -o $podir/plasma_login.pot

xgettext \
    --language=Rust \
    --keyword=gettext --keyword=ngettext:1,2 \
    --from-code=UTF-8 --sort-by-file \
    --join-existing \
    -o "$podir/plasma_login.pot" \
    $(find "$podir/../src/failure/" -name \*.rs)
