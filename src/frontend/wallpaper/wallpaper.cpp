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
    polish();
}

void Wallpaper::updatePolish()
{
    if (m_wallpaperItem) {
        return;
    }
    auto wallpaperPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    wallpaperPackage.setPath(PlasmaLoginSettings::getInstance().wallpaperPluginId());

    if (!wallpaperPackage.isValid()) {
        qWarning() << "Error loading the wallpaper, not a valid package";
        return;
    }

    const QString xmlPath = wallpaperPackage.filePath(QByteArrayLiteral("config"), QStringLiteral("main.xml"));

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

    const QUrl sourceUrl = QUrl::fromLocalFile(wallpaperPackage.filePath("mainscript"));

    auto engine = qmlEngine(this);

    auto *component = new QQmlComponent(engine, sourceUrl, this);
    if (component->isError()) {
        qWarning() << "Failed to load wallpaper component:" << component->errors();
        return;
    }

    const QVariantMap properties = {{QStringLiteral("configuration"), QVariant::fromValue(config)},
                                    {QStringLiteral("pluginName"), PlasmaLoginSettings::getInstance().wallpaperPluginId()}};
    QObject *wallpaperObject = component->createWithInitialProperties(properties);
    m_wallpaperItem = qobject_cast<QQuickItem *>(wallpaperObject);
    if (!m_wallpaperItem) {
        qWarning() << "Failed to create wallpaper root object:" << component->errors();
        return;
    }

    m_wallpaperItem->setParentItem(this);
    m_wallpaperItem->setSize(size());
}

void Wallpaper::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (!m_wallpaperItem) {
        return;
    }
    if (change == ItemTransformHasChanged) {
        m_wallpaperItem->setSize(size());
    }
    QQuickItem::itemChange(change, value);
}
