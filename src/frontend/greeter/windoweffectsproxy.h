/*
 * SPDX-FileContributor: 2025 Oliver Beard <olib141@outlook.com
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <qqmlregistration.h>
#include <QObject>
#include <QQuickWindow>

class WindowEffectsProxy : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickWindow *window MEMBER m_window WRITE setWindow NOTIFY windowChanged)

public:
    explicit WindowEffectsProxy(QObject *parent = nullptr);

    void setWindow(QQuickWindow *window);

    Q_INVOKABLE void setBlurBehind(qreal radius);
    Q_INVOKABLE void setBackgroundContrast(qreal contrast, qreal intensity, qreal saturation);

signals:
    void windowChanged();

private:
    QQuickWindow *m_window;
};
