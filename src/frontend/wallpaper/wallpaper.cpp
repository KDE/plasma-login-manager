/*
 * SPDX-FileCopyrightText: 2025 Oliver Beard
 * SPDX-FileCopyrightText: 2025 2026 David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "wallpaper.h"

#include <KConfigGroup>
#include <KConfigLoader>
#include <KConfigPropertyMap>

#include <KPackage/PackageLoader>

#include <QEvent>
#include <QFile>
#include <QQuickWindow>

#include "plasmaloginsettings.h"

Wallpaper::Wallpaper(QQuickItem *parent)
    : QQuickItem(parent)
{
}

// deferred out of the constructor as we need the engine
void Wallpaper::componentComplete()
{
    QQuickItem::componentComplete();

    auto m_wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    m_wallpaperPackage.setPath(PlasmaLoginSettings::getInstance().wallpaperPluginId());

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

    auto engine = qmlEngine(this);

    auto *component = new QQmlComponent(engine, sourceUrl, this);
    if (component->isError()) {
        qWarning() << "Failed to load wallpaper component:" << component->errors();
        return;
    }

    const QVariantMap properties = {{QStringLiteral("configuration"), QVariant::fromValue(config)},
                                    {QStringLiteral("pluginName"), PlasmaLoginSettings::getInstance().wallpaperPluginId()}};
    QObject *wallpaperObject = component->createWithInitialProperties(properties);
    auto wallpaperItem = qobject_cast<QQuickItem *>(wallpaperObject);
    if (!wallpaperItem) {
        qWarning() << "Failed to create wallpaper root object:" << component->errors();
        return;
    }

    wallpaperItem->setParentItem(this);
    wallpaperItem->setWidth(width());
    wallpaperItem->setHeight(height());

    connect(this, &QQuickItem::widthChanged, wallpaperItem, [this, wallpaperItem]() {
        wallpaperItem->setWidth(width());
    });
    connect(this, &QQuickItem::heightChanged, wallpaperItem, [this, wallpaperItem]() {
        wallpaperItem->setHeight(height());
    });
}
