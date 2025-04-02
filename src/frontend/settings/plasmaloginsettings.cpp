/*
 *  SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *  SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <limits>

#include <QCollator>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include "config.h"

#include "plasmaloginsettings.h"

PlasmaLoginSettings &PlasmaLoginSettings::getInstance()
{
    auto config = KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_CONFIG_FILE), KConfig::CascadeConfig);

    static PlasmaLoginSettings instance(config);
    return instance;
}

PlasmaLoginSettings::PlasmaLoginSettings(KSharedConfig::Ptr config, QObject *parent)
    : PlasmaLoginSettingsBase(config)
{
    setParent(parent);

    getUids();
    getWallpaperPlugins();
}

PlasmaLoginSettings::~PlasmaLoginSettings()
{
}

void PlasmaLoginSettings::getUids()
{
    m_minimumUid = std::numeric_limits<unsigned int>::min();
    m_maximumUid = std::numeric_limits<unsigned int>::max();

    QFile loginDefs(QStringLiteral("/etc/login.defs"));
    if (!loginDefs.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to determine min/max uid";
        return;
    }

    QTextStream in(&loginDefs);
    const QStringList keys = {QStringLiteral("UID_MIN"), QStringLiteral("UID_MAX")};

    while (!in.atEnd()) {
        QString line = in.readLine().split(QLatin1Char('#')).first().simplified();
        if (!line.isEmpty()) {
            QStringList lineParts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);

            if (lineParts.size() != 2 || !keys.contains(lineParts[0])) {
                continue;
            }

            bool ok;
            unsigned int value = lineParts[1].toUInt(&ok);
            if (ok) {
                if (lineParts[0] == QStringLiteral("UID_MIN")) {
                    m_minimumUid = value;
                } else {
                    m_maximumUid = value;
                }
            }
        }
    }
}

void PlasmaLoginSettings::getWallpaperPlugins()
{
    const auto wallpaperPackages = KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma/Wallpaper"));
    for (auto &package : wallpaperPackages) {
        m_availableWallpaperPlugins.append({package.name(), package.pluginId()});
    }

    QCollator collator; // TODO: This isn't even used?
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_availableWallpaperPlugins.begin(), m_availableWallpaperPlugins.end(), [](const WallpaperInfo &w1, const WallpaperInfo &w2) {
        return w1.name < w2.name;
    });
}

unsigned int PlasmaLoginSettings::minimumUid() const
{
    return m_minimumUid;
}

unsigned int PlasmaLoginSettings::maximumUid() const
{
    return m_maximumUid;
}

QList<WallpaperInfo> PlasmaLoginSettings::availableWallpaperPlugins() const
{
    return m_availableWallpaperPlugins;
}

#include "moc_plasmaloginsettings.cpp"
