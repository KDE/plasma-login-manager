/*
 *  SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
 *  SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
 *  SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *  SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KConfigPropertyMap>
#include <KQuickManagedConfigModule>

#include "plasmalogindata.h"

class UserModel;
class SessionModel;

class PlasmaLoginKcm : public KQuickManagedConfigModule
{
    Q_OBJECT
public:
    explicit PlasmaLoginKcm(QObject *parent, const KPluginMetaData &data);

    Q_PROPERTY(PlasmaLoginSettings *settings READ settings CONSTANT)
    Q_PROPERTY(KConfigPropertyMap *wallpaperConfiguration READ wallpaperConfiguration NOTIFY currentWallpaperChanged)
    Q_PROPERTY(QUrl wallpaperConfigFile READ wallpaperConfigFile NOTIFY currentWallpaperChanged)
    Q_PROPERTY(WallpaperIntegration *wallpaperIntegration READ wallpaperIntegration NOTIFY currentWallpaperChanged)
    Q_PROPERTY(QString currentWallpaper READ currentWallpaper NOTIFY currentWallpaperChanged)
    Q_PROPERTY(UserModel *userModel READ userModel CONSTANT)
    Q_PROPERTY(SessionModel *sessionModel READ sessionModel CONSTANT)
    Q_PROPERTY(QString currentUser READ currentUser CONSTANT)

    // TODO: Why not use directly? Could expose in PlasmaLoginSettings as Q_INVOKABLE
    Q_INVOKABLE QList<WallpaperInfo> availableWallpaperPlugins()
    {
        return PlasmaLoginSettings::self()->availableWallpaperPlugins();
    }

    Q_INVOKABLE void synchronizeSettings();
    Q_INVOKABLE void resetSynchronizedSettings();
    Q_INVOKABLE bool KDEWalletAvailable();
    Q_INVOKABLE void openKDEWallet();

    PlasmaLoginSettings *settings() const;
    QUrl wallpaperConfigFile() const;
    WallpaperIntegration *wallpaperIntegration() const;
    QString currentWallpaper() const;
    UserModel *userModel() const;
    SessionModel *sessionModel() const;
    QString currentUser() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void errorOccurred(const QString &untranslatedMessage);
    void syncAttempted();

    void currentWallpaperChanged();

private:
    bool isSaveNeeded() const override;
    bool isDefaults() const override;

    QVariantMap syncWallpaper(const QUrl &url);
    KConfigPropertyMap *wallpaperConfiguration() const;

    PlasmaLoginData *m_data;

    QString m_currentWallpaper;
};
