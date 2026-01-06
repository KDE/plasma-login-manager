/*
 *  SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
 *  SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *  SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QList>
#include <QTemporaryDir>
#include <QTextStream>

#include <KAuth/ExecuteJob>
#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KIO/ApplicationLauncherJob>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KService>
#include <KUser>
#include <kauth/action.h>
#include <qdbusunixfiledescriptor.h>

#include "models/sessionmodel.h"
#include "models/usermodel.h"
#include "plasmalogindata.h"
#include "wallpapersettings.h"

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
    qmlRegisterAnonymousType<UserModel>(url, 1);
    qmlRegisterAnonymousType<SessionModel>(url, 1);
    qmlProtectModule(url, 1);

    // Our modules will be checking the Plasmoid attached object when running from Plasma, let it load the module
    constexpr const char *uri = "org.kde.plasma.plasmoid";
    qmlRegisterUncreatableType<QObject>(uri, 2, 0, "PlasmoidPlaceholder", QStringLiteral("Do not create objects of type Plasmoid"));

    connect(&PlasmaLoginSettings::getInstance(), &PlasmaLoginSettings::WallpaperPluginIdChanged, m_wallpaperSettings, &WallpaperSettings::loadWallpaperConfig);
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
    // We are not allowed to write GUI items to the arg map passed to KAuth, such as QColor
    // which is used in most wallpapers. So instead, we'll have to save a temporary copy of
    // the written-out config and have KAuth update the installed file with its content.

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        Q_EMIT errorOccurred(QString::fromUtf8(kli18n("Unable to save settings because a temporary directory could not be created.").untranslatedText()));
        return;
    }

    const QString tempFileName = tempDir.path() + QLatin1String("/plasma-login.conf");
    KConfig tempConfig(tempFileName, KConfig::SimpleConfig);

    // Write our config
    for (const auto &item : PlasmaLoginSettings::getInstance().items()) {
        if (!item->isDefault()) {
            // Write this to the new config
            tempConfig.group(item->group()).writeEntry(item->key(), item->property());
        }
    }

    // Write wallpaper config
    const QString wallpaperPluginId = PlasmaLoginSettings::getInstance().wallpaperPluginId();
    for (const QString &wallpaperKey : wallpaperConfiguration()->keys()) {
        const QVariant value = wallpaperConfiguration()->value(wallpaperKey);
        tempConfig.group(QLatin1String("Greeter"))
            .group(QLatin1String("Wallpaper"))
            .group(wallpaperPluginId)
            .group(QLatin1String("General"))
            .writeEntry(wallpaperKey, value);
    }

    tempConfig.sync();

    // Open our temporary saved config
    QFile tempFile(tempFileName);
    if (!tempFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Q_EMIT errorOccurred(QString::fromUtf8(kli18n("Unable to save settings because the config could not be opened.").untranslatedText()));
        return;
    }

    QVariantMap args;
    QTextStream in(&tempFile);
    args[QStringLiteral("config")] = in.readAll();

    // For image wallpapers we want to copy the user-set image
    auto imageWallpaperGroup = tempConfig.group("Greeter").group("Wallpaper").group("org.kde.image");
    if (imageWallpaperGroup.exists()) {
        const QUrl imageUri = imageWallpaperGroup.group("General").readEntry("Image");

        const QString imagePath = imageUri.toLocalFile(); //Dave, should we do KIO stuff?
        // we read the file here to ensure that we can read the contents
        // we could pass an FD, but changing wallpaper shouldn't be a high frequency operation

        qDebug() << imageUri << imagePath;
        if (!imagePath.isEmpty()) {
            QFile imageFile(imagePath);
            if (imageFile.open(QIODevice::ReadOnly)) {
                args[QStringLiteral("wallpaper")] = imageFile.readAll();
                qDebug() << args[QStringLiteral("wallpaper")].toByteArray().size();
                args[QStringLiteral("wallpaperFd")] = QVariant::fromValue(QDBusUnixFileDescriptor(imageFile.handle()));
            } else {
                qDebug() << "Could not read file";
            }
        }
    }

    KAuth::Action saveAction(authActionName());
    saveAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));
    saveAction.setArguments(args);

    auto job = saveAction.execute();
    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            Q_EMIT errorOccurred(job->errorString());
        } else {
            updateState();
        }
        // Clarify enable or disable the Apply button.
        this->setNeedsSave(job->error());
    });
    job->start();
}

void PlasmaLoginKcm::synchronizeSettings()
{
    if (KUser("plasmalogin").homeDir().isEmpty()) {
        Q_EMIT errorOccurred(QString::fromUtf8(
            kli18n("Unable to synchronise Plasma settings because the 'plasmalogin' user does not exist. Please check your Plasma Login install.")
                .untranslatedText()));
        return;
    }

    QVariantMap args;

    auto addConfigFile = [&args](const QString &path, const QString &key) {
        if (path.isEmpty()) {
            return;
        }

        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&file);
            args[key] = in.readAll();
        }
    };

    addConfigFile(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kdeglobals")), QStringLiteral("kdeglobals"));

    addConfigFile(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("plasmarc")), QStringLiteral("plasmarc"));

    addConfigFile(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kcminputrc")), QStringLiteral("kcminputrc"));

    addConfigFile(QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kwinoutputconfig.json")),
                  QStringLiteral("kwinoutputconfig.json"));

    const QString fontconfigPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("fontconfig"), QStandardPaths::LocateDirectory);
    if (!fontconfigPath.isEmpty()) {
        addConfigFile(fontconfigPath + QStringLiteral("/fonts.conf"), QStringLiteral("fontconfig/fonts.conf"));
    }

    KAuth::Action syncAction(QStringLiteral("org.kde.kcontrol.kcmplasmalogin.sync"));
    syncAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));
    syncAction.setArguments(args);

    auto job = syncAction.execute();
    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            Q_EMIT errorOccurred(job->errorString());
        }
        Q_EMIT syncAttempted();
    });
    job->start();
}

void PlasmaLoginKcm::resetSynchronizedSettings()
{
    if (KUser("plasmalogin").homeDir().isEmpty()) {
        Q_EMIT errorOccurred(
            QString::fromUtf8(kli18n("Unable to reset Plasma settings because the 'plasmalogin' user does not exist. Please check your Plasma Login install.")
                                  .untranslatedText()));
        return;
    }

    KAuth::Action resetAction(QStringLiteral("org.kde.kcontrol.kcmplasmalogin.reset"));
    resetAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));

    auto job = resetAction.execute();
    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            Q_EMIT errorOccurred(job->errorString());
        }
        Q_EMIT syncAttempted();
    });
    job->start();
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

UserModel *PlasmaLoginKcm::userModel() const
{
    static UserModel userModel;
    return &userModel;
}

SessionModel *PlasmaLoginKcm::sessionModel() const
{
    static SessionModel sessionModel;
    return &sessionModel;
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
