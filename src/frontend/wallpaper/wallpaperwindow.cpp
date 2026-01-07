/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTimer>

#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <KWindowSystem>

#include <LayerShellQt/Window>

#include "wallpaperwindow.h"

WallpaperWindow::WallpaperWindow(QScreen *screen)
    : PlasmaQuick::QuickViewSharedEngine()
    , m_screen(screen)
{
    if (KWindowSystem::isPlatformWayland()) {
        if (auto layerShellWindow = LayerShellQt::Window::get(this)) {
            layerShellWindow->setScope(QStringLiteral("plasma-login-wallpaper"));
            layerShellWindow->setLayer(LayerShellQt::Window::LayerBackground);
            layerShellWindow->setExclusiveZone(-1);
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        }
    }

    setColor(Qt::black);
    setScreen(m_screen);

    setGeometry(m_screen->geometry());
    connect(m_screen, &QScreen::geometryChanged, this, [this]() {
        setGeometry(m_screen->geometry());
    });

    setResizeMode(PlasmaQuick::QuickViewSharedEngine::SizeRootObjectToView);

    if (KWindowSystem::isPlatformX11()) {
        // X11 specific hint only on X11
        setFlags(Qt::BypassWindowManagerHint);
    } else if (!KWindowSystem::isPlatformWayland()) {
        // on other platforms go fullscreen
        // on Wayland we cannot go fullscreen due to QTBUG 54883
        setWindowState(Qt::WindowFullScreen);
    }
}

bool WallpaperWindow::blur() const
{
    return m_blur;
}

void WallpaperWindow::setBlur(bool enable)
{
    if (m_blur == enable) {
        return;
    }

    m_blur = enable;
    Q_EMIT blurChanged();
}
