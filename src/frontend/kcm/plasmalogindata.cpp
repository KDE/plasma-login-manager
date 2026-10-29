/*
 *  SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

#include "plasmalogindata.h"

PlasmaLoginData::PlasmaLoginData(QObject *parent)
    : KCModuleData(parent)
    , m_plasmaLoginSettings(PlasmaLoginSettings::self())
    , m_wallpaperSettings(new WallpaperSettings(this))
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                      QStringLiteral("/org/freedesktop/systemd1"),
                                                      QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                      QStringLiteral("GetUnitFileState"));
    msg << QStringLiteral("plasmalogin.service");

    // it seems system settings wants things to be sync
    // It's not like systemd will ever be down
    QDBusReply<QString> hasPlasmaLoginReply = QDBusConnection::systemBus().call(msg);
    // a quirk is that if plasmalogin is uninstalled systemd replies with an error of invalid args
    if (hasPlasmaLoginReply.value() == QLatin1String("enabled")) {
        setRelevant(true);
    } else {
        setRelevant(false);
    }

    m_plasmaLoginSettings->load();
    m_wallpaperSettings->load(m_plasmaLoginSettings);

    connect(m_plasmaLoginSettings, &PlasmaLoginSettings::WallpaperPluginIdChanged, m_wallpaperSettings, [this]() {
        m_wallpaperSettings->loadWallpaperConfig(m_plasmaLoginSettings);
    });
    m_wallpaperSettings->loadWallpaperConfig(m_plasmaLoginSettings);

    // We can't register skeletons because we can't unregister the wallpaper skeleton
    // when it's replaced on loadWallpaperConfig
    /*
    registerSkeleton(m_plasmaLoginSettings);
    registerSkeleton(m_wallpaperSettings->wallpaperSkeleton());
    */
}

PlasmaLoginSettings *PlasmaLoginData::plasmaLoginSettings() const
{
    return m_plasmaLoginSettings;
}

WallpaperSettings *PlasmaLoginData::wallpaperSettings() const
{
    return m_wallpaperSettings;
}

bool PlasmaLoginData::isDefaults() const
{
    return m_plasmaLoginSettings->isDefaults() && m_wallpaperSettings->isDefaults();
}

void PlasmaLoginData::revertToDefaults()
{
    // TODO: Saving here from KCModuleData impl, but probably not necessary

    m_plasmaLoginSettings->useDefaults(true);
    m_plasmaLoginSettings->save();

    m_wallpaperSettings->wallpaperSkeleton()->useDefaults(true);
    m_wallpaperSettings->wallpaperSkeleton()->save();
}

#include "moc_plasmalogindata.cpp"
