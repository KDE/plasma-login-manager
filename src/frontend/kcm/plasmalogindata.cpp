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

    m_wallpaperSettings->load();

    registerSkeleton(m_plasmaLoginSettings);
    registerSkeleton(m_wallpaperSettings->wallpaperSkeleton()); // TODO: Does the skeleton change on wallpaper plugin change?
}

PlasmaLoginSettings *PlasmaLoginData::plasmaLoginSettings() const
{
    return m_plasmaLoginSettings;
}

WallpaperSettings *PlasmaLoginData::wallpaperSettings() const
{
    return m_wallpaperSettings;
}

#include "moc_plasmalogindata.cpp"
