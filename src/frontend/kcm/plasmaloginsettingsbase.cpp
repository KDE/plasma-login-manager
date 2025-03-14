/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "plasmaloginsettingsbase.h"

#include "config.h"

#include <QDebug>
#include <QDir>

#include <algorithm>

PlasmaLoginSettingsBase::PlasmaLoginSettingsBase(KSharedConfigPtr config, QObject *parent)
    : KConfigSkeleton(config, parent)
{
    auto defaultFiles = QDir(QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_DIR)).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::LocaleAware);
    std::transform(defaultFiles.begin(), defaultFiles.end(), defaultFiles.begin(), [](const QString &filename) -> QString {
        return QStringLiteral(PLASMALOGIN_SYSTEM_CONFIG_DIR "/") + filename;
    });
    // If no filename is set, KConfig will not parse any file
    if (!defaultFiles.isEmpty()) {
        m_defaultConfig = KSharedConfig::openConfig(defaultFiles.takeLast(), KConfig::CascadeConfig);
    } else {
        m_defaultConfig = KSharedConfig::openConfig(QString(), KConfig::CascadeConfig);
    }
    m_defaultConfig->addConfigSources(defaultFiles);
}

QString PlasmaLoginSettingsBase::defaultWallpaperPluginId() const
{
    return m_defaultConfig->group(QStringLiteral("Greeter")).readEntry("WallpaperPluginId", "org.kde.image");
}

unsigned int PlasmaLoginSettingsBase::defaultMinimumUid() const
{
    return m_defaultConfig->group(QStringLiteral("Users")).readEntry("MinimumUid", 1000);
}

unsigned int PlasmaLoginSettingsBase::defaultMaximumUid() const
{
    return m_defaultConfig->group(QStringLiteral("Users")).readEntry("MaximumUid", 60000);
}

QString PlasmaLoginSettingsBase::defaultUser() const
{
    return m_defaultConfig->group(QStringLiteral("AutoLogin")).readEntry("User");
}

QString PlasmaLoginSettingsBase::defaultSession() const
{
    return m_defaultConfig->group(QStringLiteral("AutoLogin")).readEntry("Session");
}

bool PlasmaLoginSettingsBase::defaultRelogin() const
{
    return m_defaultConfig->group(QStringLiteral("AutoLogin")).readEntry("Relogin", false);
}

QString PlasmaLoginSettingsBase::defaultHaltCommand() const
{
    return m_defaultConfig->group(QStringLiteral("General")).readEntry("HaltCommand");
}

QString PlasmaLoginSettingsBase::defaultRebootCommand() const
{
    return m_defaultConfig->group(QStringLiteral("General")).readEntry("RebootCommand");
}
