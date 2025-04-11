/*
 * SPDX-FileContributor: 2025 Oliver Beard <olib141@outlook.com
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KWindowEffects>

#include "windoweffectsproxy.h"

WindowEffectsProxy::WindowEffectsProxy(QObject *parent)
    : QObject(parent)
    , m_window(nullptr)
{
}

void WindowEffectsProxy::setWindow(QQuickWindow *window)
{
    m_window = window;
    emit windowChanged();
}

void WindowEffectsProxy::setBlurBehind(qreal radius)
{
    if (!m_window) {
        return;
    }

    Q_UNUSED(radius); // TODO: enableBlurBehind needs to optionally take radius
    KWindowEffects::enableBlurBehind(m_window, true);
}

void WindowEffectsProxy::setBackgroundContrast(qreal contrast, qreal intensity, qreal saturation)
{
    if (!m_window) {
        return;
    }

    KWindowEffects::enableBackgroundContrast(m_window, true, contrast, intensity, saturation);
}

#include "moc_windoweffectsproxy.cpp"
