/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTimer>

#include <KWindowSystem>
#include <KPackage/Package>
#include <KPackage/PackageLoader>

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
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
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

    QTimer::singleShot(5000, this, &QWindow::close); // TODO: Remove when appropriate
}
