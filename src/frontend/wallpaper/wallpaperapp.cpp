/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>

#include <QDBusConnection>
#include <QDBusError>

#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KLocalizedQmlContext>
#include <KPackage/PackageLoader>
#include <KWindowSystem>
#include <LayerShellQt/Window>
#include <PlasmaQuick/PlasmaQuick>

#include "plasmaloginsettings.h"

#include "wallpaperwindow.h"

#include "wallpaperapp.h"

WallpaperApp::WallpaperApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
    , m_engine(PlasmaQuick::globalEngine())
{
    KLocalization::setupLocalizedContext(m_engine.get());

    m_wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    m_wallpaperPackage.setPath(PlasmaLoginSettings::getInstance().wallpaperPluginId());

    connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        createWindowForScreen(screen);
    });
    for (QScreen *screen : qApp->screens()) {
        createWindowForScreen(screen);
    }

    auto bus = QDBusConnection::sessionBus();
    bus.registerObject(QStringLiteral("/Wallpaper"), this, QDBusConnection::ExportScriptableSlots);
    if (!bus.registerService(QStringLiteral("org.kde.plasma.wallpaper"))) {
        qWarning() << "Failed to register DBus service org.kde.plasma.wallpaper:" << bus.lastError().message();
    }
}

void WallpaperApp::createWindowForScreen(QScreen *screen)
{
    WallpaperWindow *window = new WallpaperWindow(m_engine.get());
    window->QObject::setParent(this);
    window->setScreen(screen);
    window->setColor(Qt::black);

    connect(qApp, &QGuiApplication::screenRemoved, window, [window](QScreen *screenRemoved) {
        if (screenRemoved == window->screen()) {
            delete window;
        }
    });

    window->setGeometry(screen->geometry());

    if (KWindowSystem::isPlatformWayland()) {
        if (auto layerShellWindow = LayerShellQt::Window::get(window)) {
            layerShellWindow->setScope(QStringLiteral("plasma-login-wallpaper"));
            layerShellWindow->setLayer(LayerShellQt::Window::LayerBackground);
            layerShellWindow->setExclusiveZone(-1);
            layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            layerShellWindow->setScreen(screen);
        }
    }

    window->setResizeMode(QQuickView::SizeRootObjectToView);

    if (KWindowSystem::isPlatformX11()) {
        // X11 specific hint only on X11
        window->setFlags(Qt::BypassWindowManagerHint);
    } else if (!KWindowSystem::isPlatformWayland()) {
        // on other platforms go fullscreen
        // on Wayland we cannot go fullscreen due to QTBUG 54883
        window->setWindowState(Qt::WindowFullScreen);
    }

    setupWallpaperPlugin(window);
    window->show();
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

    const QUrl sourceUrl = QUrl::fromLocalFile(m_wallpaperPackage.filePath("mainscript"));

    auto *component = new QQmlComponent(window->engine(), sourceUrl, window);
    if (component->isError()) {
        qWarning() << "Failed to load wallpaper component:" << component->errors();
        return;
    }

    window->setSource(QUrl(QStringLiteral("qrc:/qt/qml/org/kde/plasma/login/wallpaper/main.qml")));

    const QVariantMap properties = {{QStringLiteral("configuration"), QVariant::fromValue(config)},
                                    {QStringLiteral("pluginName"), PlasmaLoginSettings::getInstance().wallpaperPluginId()}};
    QObject *wallpaperObject = component->createWithInitialProperties(properties, window->rootContext());
    auto wallpaperItem = qobject_cast<QQuickItem *>(wallpaperObject);
    if (!wallpaperItem) {
        qWarning() << "Failed to create wallpaper root object:" << component->errors();
        return;
    }
    auto wallpaperContainer = window->rootObject()->property("wallpaperContainer").value<QQuickItem *>();

    wallpaperItem->setParentItem(wallpaperContainer);
    wallpaperItem->setWidth(wallpaperContainer->width());
    wallpaperItem->setHeight(wallpaperContainer->height());
    connect(wallpaperContainer, &QQuickItem::widthChanged, wallpaperItem, [wallpaperContainer, wallpaperItem]() {
        wallpaperItem->setWidth(wallpaperContainer->width());
    });
    connect(wallpaperContainer, &QQuickItem::heightChanged, wallpaperItem, [wallpaperContainer, wallpaperItem]() {
        wallpaperItem->setHeight(wallpaperContainer->height());
    });
}

void WallpaperApp::blurScreen(const QString &screenName)
{
    for (QWindow *window : topLevelWindows()) {
        WallpaperWindow *wallpaperWindow = qobject_cast<WallpaperWindow *>(window);
        if (!wallpaperWindow) {
            continue;
        }

        if (wallpaperWindow->screen()->name() == screenName) {
            wallpaperWindow->setBlur(true);
        } else {
            wallpaperWindow->setBlur(false);
        }
    }
}

#include "moc_wallpaperapp.cpp"
