/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "bluradaptor.h"

#include <QDBusConnection>
#include <QDBusError>

BlurAdaptor::BlurAdaptor(QObject *parent)
    : QObject(parent)
{
    auto bus = QDBusConnection::sessionBus();
    bus.registerObject(QStringLiteral("/Wallpaper"), this, QDBusConnection::ExportScriptableSlots);
    if (!bus.registerService(QStringLiteral("org.kde.plasma.wallpaper"))) {
        qWarning() << "Failed to register DBus service org.kde.plasma.wallpaper:" << bus.lastError().message();
    }
}

void BlurAdaptor::blurScreen(const QString &screenName)
{
    m_activeScreen = screenName;
    Q_EMIT activeScreenChanged();
}
