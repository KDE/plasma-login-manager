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

    auto bus = QDBusConnection::sessionBus();
    bus.registerObject(QStringLiteral("/Wallpaper"), this, QDBusConnection::ExportScriptableSlots);
    if (!bus.registerService(QStringLiteral("org.kde.plasma.wallpaper"))) {
        qWarning() << "Failed to register DBus service org.kde.plasma.wallpaper:" << bus.lastError().message();
    }
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

    const QUrl sourceUrl = QUrl::fromLocalFile(m_wallpaperPackage.filePath("mainscript"));

    auto *component = new QQmlComponent(window->engine().get(), sourceUrl, window);
    if (component->isError()) {
        qWarning() << "Failed to load wallpaper component:" << component->errors();
        return;
    }

    const QVariantMap properties = {{QStringLiteral("configuration"), QVariant::fromValue(config)},
                                    {QStringLiteral("pluginName"), PlasmaLoginSettings::getInstance().wallpaperPluginId()}};
    QObject *wallpaperObject = component->createWithInitialProperties(properties, window->rootContext());
    auto wallpaperItem = qobject_cast<QQuickItem *>(wallpaperObject);
    if (!wallpaperItem) {
        qWarning() << "Failed to create wallpaper root object:" << component->errors();
        return;
    }
    wallpaperItem->setParentItem(window->contentItem());
    wallpaperItem->setWidth(window->contentItem()->width());
    wallpaperItem->setHeight(window->contentItem()->height());
    connect(window->contentItem(), &QQuickItem::widthChanged, wallpaperItem, [contentItem = window->contentItem(), wallpaperItem]() {
        wallpaperItem->setWidth(contentItem->width());
    });
    connect(window->contentItem(), &QQuickItem::heightChanged, wallpaperItem, [contentItem = window->contentItem(), wallpaperItem]() {
        wallpaperItem->setHeight(contentItem->height());
    });
}

void WallpaperApp::blurScreen(const QString &screenName)
{
    for (WallpaperWindow *window : std::as_const(m_windows)) {
        if (window->screen()->name() == screenName) {
            window->setBlur(true);
        } else {
            window->setBlur(false);
        }
    }
}

#include "wallpaperapp.moc"

#include "moc_wallpaperapp.cpp"
