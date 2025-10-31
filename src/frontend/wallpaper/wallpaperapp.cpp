/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QQmlContext>
#include <QQuickItem>

#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KPackage/PackageLoader>
#include <KWindowSystem>

#include <LayerShellQt/Shell>

#include "plasmaloginsettings.h"

#include "wallpaperwindow.h"

#include "wallpaperapp.h"

WallpaperApp::WallpaperApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
{
    LayerShellQt::Shell::useLayerShell();

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

    connect(screen, &QScreen::geometryChanged, this, [window](const QRect &geometry) {
        window->setGeometry(geometry);
    });

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

    const QString xmlPath = m_wallpaperPackage.filePath(QByteArrayLiteral("config"), QStringLiteral("main.xml"));

    const KConfigGroup cfg = PlasmaLoginSettings::getInstance()
                                 .sharedConfig()
                                 ->group(QStringLiteral("Greeter"))
                                 .group(QStringLiteral("Wallpaper"))
                                 .group(PlasmaLoginSettings::getInstance().wallpaperPluginId());

    KConfigLoader *configLoader;
    if (xmlPath.isEmpty()) {
        configLoader = new KConfigLoader(cfg, nullptr, this);
    } else {
        QFile file(xmlPath);
        configLoader = new KConfigLoader(cfg, &file, this);
    }

    KConfigPropertyMap *config = new KConfigPropertyMap(configLoader, this);
    // potd (picture of the day) is using a kded to monitor changes and
    // cache data for the lockscreen. Let's notify it.
    config->setNotify(true);

    window->setSource(QUrl::fromLocalFile(m_wallpaperPackage.filePath("mainscript")));
    window->rootObject()->setProperty("configuration", QVariant::fromValue(config));
    window->rootObject()->setProperty("pluginName", PlasmaLoginSettings::getInstance().wallpaperPluginId());
}

#include "wallpaperapp.moc"

#include "moc_wallpaperapp.cpp"
