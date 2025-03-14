/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#include "plasmalogindata.h"

#include "config.h"
#include "plasmaloginsettings.h"

#include <KSharedConfig>

#include <QDir>

PlasmaLoginData::PlasmaLoginData(QObject *parent)
    : KCModuleData(parent)
{
    auto config = KSharedConfig::openConfig(QStringLiteral(PLASMALOGIN_CONFIG_FILE), KConfig::CascadeConfig);
    QStringList configFiles = QDir(QStringLiteral(PLASMALOGIN_CONFIG_DIR)).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::LocaleAware);
    std::transform(configFiles.begin(), configFiles.end(), configFiles.begin(), [](const QString &filename) -> QString {
        return QStringLiteral(PLASMALOGIN_CONFIG_DIR "/") + filename;
    });
    config->addConfigSources(configFiles);
    m_settings = new PlasmaLoginSettings(config, this);
    autoRegisterSkeletons();
}

PlasmaLoginSettings *PlasmaLoginData::plasmaLoginSettings() const
{
    return m_settings;
}
