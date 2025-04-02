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

#include "plasmaloginsettings.h"
#include "wallpaperintegration.h"

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
    Q_PROPERTY(bool isDefaultsWallpaper READ isDefaultsWallpaper NOTIFY isDefaultsWallpaperChanged)
    Q_PROPERTY(UserModel *userModel READ userModel CONSTANT)
    Q_PROPERTY(SessionModel *sessionModel READ sessionModel CONSTANT)

    // TODO: Why not use directly? Could expose in PlasmaLoginSettings as Q_INVOKABLE
    Q_INVOKABLE QList<WallpaperInfo> availableWallpaperPlugins()
    {
        return PlasmaLoginSettings::getInstance().availableWallpaperPlugins();
    }

    Q_INVOKABLE void synchronizeSettings();
    Q_INVOKABLE void resetSynchronizedSettings();
    Q_INVOKABLE bool KDEWalletAvailable();
    Q_INVOKABLE void openKDEWallet();

    PlasmaLoginSettings *settings() const;
    QUrl wallpaperConfigFile() const;
    WallpaperIntegration *wallpaperIntegration() const;
    QString currentWallpaper() const;
    bool isDefaultsWallpaper() const;
    UserModel *userModel() const;
    SessionModel *sessionModel() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;
    void updateState();
    void forceUpdateState();

Q_SIGNALS:
    void errorOccurred(const QString &untranslatedMessage);
    void syncAttempted();

    /**
     * Emitted when the defaults function is called.
     */
    void defaultsCalled();

    void isDefaultsWallpaperChanged();

    void currentWallpaperChanged();

    /**
     * Emitted when the load function is called.
     */
    void loadCalled();

private:
    bool isSaveNeeded() const override;
    bool isDefaults() const override;

    KConfigPropertyMap *wallpaperConfiguration() const;

    WallpaperSettings *m_wallpaperSettings;
    QString m_currentWallpaper;
    bool m_forceUpdateState = false;
};
