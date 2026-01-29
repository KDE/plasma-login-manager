/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <PlasmaQuick/QuickViewSharedEngine>

class GreeterEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit GreeterEventFilter(QObject *parent = nullptr);

Q_SIGNALS:
    void keyPressed();
    void escapeKeyPressed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
