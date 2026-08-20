// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include "Arbiter.h"

#include <QDebug>

using namespace Qt::StringLiterals;
using namespace PLASMALOGIN;

[[nodiscard]] QString Arbiter::GetUser() const
{
    return u"me"_s;
}

void PLASMALOGIN::Arbiter::Result(int res)
{
    qWarning() << "result" << res;
    Q_EMIT result(GetUser(), res == 1);
}
