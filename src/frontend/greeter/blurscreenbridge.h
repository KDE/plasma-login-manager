/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QQuickWindow>

class BlurScreenBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickWindow *activeWindow READ activeWindow WRITE setActiveWindow NOTIFY activeWindowChanged)

public:
    explicit BlurScreenBridge(QObject *parent = nullptr);

    void setActiveWindow(QQuickWindow *activeWindow);

    QQuickWindow *activeWindow() const;

Q_SIGNALS:
    void activeWindowChanged();

private:
    void notifyBlurScreenChange();
    QPointer<QQuickWindow> m_activeWindow = nullptr;
};
