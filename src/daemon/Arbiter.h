// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>

namespace PLASMALOGIN
{

class Arbiter : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    [[nodiscard]] QString GetUser() const;
    void Result(int result);

Q_SIGNALS:
    void result(const QString &user, bool success);
};

} // namespace PLASMALOGIN
