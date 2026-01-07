/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

#include "blurscreenbridge.h"

BlurScreenBridge::BlurScreenBridge(QObject *parent)
    : QObject(parent)
{
    QDBusServiceWatcher *watcher =
        new QDBusServiceWatcher(QStringLiteral("org.kde.plasma.wallpaper"), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BlurScreenBridge::notifyBlurScreenChange);
    connect(this, &BlurScreenBridge::activeWindowChanged, this, &BlurScreenBridge::notifyBlurScreenChange);
}

void BlurScreenBridge::setActiveWindow(QQuickWindow *activeWindow)
{
    if (m_activeWindow == activeWindow) {
        return;
    }

    m_activeWindow = activeWindow;
    Q_EMIT activeWindowChanged();
}

QQuickWindow *BlurScreenBridge::activeWindow() const
{
    return m_activeWindow;
}

void BlurScreenBridge::notifyBlurScreenChange()
{
    // Forward active window's screen to wallpaper over D-Bus for blur
    QString screenName;
    if (m_activeWindow) {
        QScreen *screen = m_activeWindow->screen();
        if (screen) {
            screenName = screen->name();
        }
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasma.wallpaper"),
                                                      QStringLiteral("/Wallpaper"),
                                                      QStringLiteral("org.kde.plasma.wallpaper"),
                                                      QStringLiteral("blurScreen"));
    msg << screenName;

    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}
