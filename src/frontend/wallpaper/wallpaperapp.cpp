/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include <KWindowSystem>
#include <KPackage/PackageLoader>

#include <LayerShellQt/Shell>

#include "wallpaperintegration.h"
#include "plasmaloginsettings.h"

#include "wallpaperwindow.h"

#include "wallpaperapp.h"

class WallpaperItem : public WallpaperIntegration
{
    Q_OBJECT
public:
    explicit WallpaperItem(QQuickItem *parent = nullptr)
        : WallpaperIntegration(parent)
    {
        setConfig(PlasmaLoginSettings::getInstance().sharedConfig());
        setPluginName(PlasmaLoginSettings::getInstance().wallpaperPluginId());
        init();
    }
};

WallpaperApp::WallpaperApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
{
    LayerShellQt::Shell::useLayerShell();

    qmlRegisterType<WallpaperItem>("org.kde.plasma.plasmoid", 2, 0, "WallpaperItem");

    m_wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    m_wallpaperPackage.setPath(PlasmaLoginSettings::getInstance().wallpaperPluginId());

    for (const auto screenList{screens()}; QScreen *screen : screenList) {
        adoptScreen(screen);
    }

    connect(this, &QGuiApplication::screenAdded, this, &WallpaperApp::adoptScreen);
}

WallpaperApp::~WallpaperApp()
{
    qDeleteAll(m_windows);
}

void WallpaperApp::adoptScreen(QScreen *screen)
{
    if (screen->geometry().isNull()) {
        return;
    }

    WallpaperWindow *window = new WallpaperWindow(screen);
    window->setGeometry(screen->geometry());
    window->setVisible(true);
    m_windows << window;

    connect(screen, &QObject::destroyed, window, [this, window]() {
        m_windows.removeAll(window);
        window->deleteLater();
    });

    setupWallpaperPlugin(window);
}

void WallpaperApp::setupWallpaperPlugin(WallpaperWindow *window)
{
    if (!m_wallpaperPackage.isValid()) {
        qWarning() << "Error loading the wallpaper, not a valid package";
        return;
    }

    window->setSource(QUrl::fromLocalFile(m_wallpaperPackage.filePath("mainscript")));
}

#include "wallpaperapp.moc"
