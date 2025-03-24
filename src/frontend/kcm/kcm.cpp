/*
 *  SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
 *  SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *  SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QList>

#include <KAuth/ExecuteJob>
#include <KIO/ApplicationLauncherJob>
#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KService>

#include "wallpapersettings.h"
#include "plasmalogindata.h"
#include "models/usermodel.h"
#include "models/sessionmodel.h"

#include "kcm.h"

K_PLUGIN_FACTORY_WITH_JSON(PlasmaLoginKcmFactory, "kcm_plasmalogin.json", registerPlugin<PlasmaLoginKcm>(); registerPlugin<PlasmaLoginData>();)

PlasmaLoginKcm::PlasmaLoginKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_wallpaperSettings(new WallpaperSettings(this))
{
    setAuthActionName(QStringLiteral("org.kde.kcontrol.kcmplasmalogin.save"));
    registerSettings(&PlasmaLoginSettings::getInstance());

    constexpr const char *url = "org.kde.private.kcms.plasmalogin";
    qRegisterMetaType<QList<WallpaperInfo>>("QList<WallpaperInfo>");
    qmlRegisterAnonymousType<PlasmaLoginSettings>(url, 1);
    qmlRegisterAnonymousType<WallpaperInfo>(url, 1);
    qmlRegisterAnonymousType<WallpaperIntegration>(url, 1);
    qmlRegisterAnonymousType<KConfigPropertyMap>(url, 1);
    qmlRegisterType<UserModel>(url, 1, 0, "UserModel");
    qmlRegisterType<SessionModel>(url, 1, 0, "SessionModel");
    qmlProtectModule(url, 1);

    // Our modules will be checking the Plasmoid attached object when running from Plasma, let it load the module
    constexpr const char *uri = "org.kde.plasma.plasmoid";
    qmlRegisterUncreatableType<QObject>(uri, 2, 0, "PlasmoidPlaceholder", QStringLiteral("Do not create objects of type Plasmoid"));

    connect(&PlasmaLoginSettings::getInstance(),
            &PlasmaLoginSettings::WallpaperPluginIdChanged,
            m_wallpaperSettings,
            &WallpaperSettings::loadWallpaperConfig);
    connect(m_wallpaperSettings, &WallpaperSettings::currentWallpaperChanged, this, &PlasmaLoginKcm::currentWallpaperChanged);
}

void PlasmaLoginKcm::load()
{
    KQuickManagedConfigModule::load();
    m_wallpaperSettings->load();

    updateState();
    Q_EMIT loadCalled();
}

void PlasmaLoginKcm::save()
{

    QVariantMap args;
    args[QStringLiteral("Autologin/User")] = PlasmaLoginSettings::getInstance().user();
    args[QStringLiteral("Autologin/Session")] = PlasmaLoginSettings::getInstance().session();
    args[QStringLiteral("Autologin/Relogin")] = PlasmaLoginSettings::getInstance().relogin();
    args[QStringLiteral("Greeter/ShowClock")] = PlasmaLoginSettings::getInstance().showClock();
    args[QStringLiteral("Greeter/WallpaperPluginId")] = PlasmaLoginSettings::getInstance().wallpaperPluginId();
    // TODO: Save relevant wallpaper group?

    KAuth::Action saveAction(authActionName());
    saveAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));
    saveAction.setArguments(args);

    auto job = saveAction.execute();
    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            Q_EMIT errorOccurred(job->errorString());
        } else {
            //m_data->plasmaLoginSettings()->load();
            updateState();
        }
        // Clarify enable or disable the Apply button.
        this->setNeedsSave(job->error());
    });
    job->start();
}

void PlasmaLoginKcm::synchronizeSettings()
{
    // TODO: Go to KAuth
}

void PlasmaLoginKcm::resetSynchronizedSettings()
{
    // TODO: Go to KAuth
}

void PlasmaLoginKcm::defaults()
{
    KQuickManagedConfigModule::defaults();
    m_wallpaperSettings->defaults();

    updateState();
    Q_EMIT defaultsCalled();
}

void PlasmaLoginKcm::updateState()
{
    m_forceUpdateState = false;
    settingsChanged();
    Q_EMIT isDefaultsWallpaperChanged();
}

void PlasmaLoginKcm::forceUpdateState()
{
    m_forceUpdateState = true;
    settingsChanged();
    Q_EMIT isDefaultsWallpaperChanged();
}

bool PlasmaLoginKcm::isSaveNeeded() const
{
    return m_forceUpdateState || m_wallpaperSettings->isSaveNeeded();
}

bool PlasmaLoginKcm::isDefaults() const
{
    return m_wallpaperSettings->isDefaults();
}

KConfigPropertyMap *PlasmaLoginKcm::wallpaperConfiguration() const
{
    return m_wallpaperSettings->wallpaperConfiguration();
}

PlasmaLoginSettings *PlasmaLoginKcm::settings() const
{
    return &PlasmaLoginSettings::getInstance();
}

QString PlasmaLoginKcm::currentWallpaper() const
{
    return PlasmaLoginSettings::getInstance().wallpaperPluginId();
}

bool PlasmaLoginKcm::isDefaultsWallpaper() const
{
    return m_wallpaperSettings->isDefaults();
}

QUrl PlasmaLoginKcm::wallpaperConfigFile() const
{
    return m_wallpaperSettings->wallpaperConfigFile();
}

WallpaperIntegration *PlasmaLoginKcm::wallpaperIntegration() const
{
    return m_wallpaperSettings->wallpaperIntegration();
}

bool PlasmaLoginKcm::KDEWalletAvailable()
{
    return !QStandardPaths::findExecutable(QLatin1String("kwalletmanager5")).isEmpty();
}

void PlasmaLoginKcm::openKDEWallet()
{
    KService::Ptr kwalletmanagerService = KService::serviceByDesktopName(QStringLiteral("org.kde.kwalletmanager5"));
    auto *job = new KIO::ApplicationLauncherJob(kwalletmanagerService);
    job->start();
}

#include "kcm.moc"

#include "moc_kcm.cpp"
