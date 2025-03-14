/*
    SPDX-FileCopyrightText: 2013 Reza Fatahilah Shah <rshah0385@kireihana.com>
    SPDX-FileCopyrightText: 2019 Filip Fila <filipfila.kde@gmail.com>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include "config.h"
#include "plasmalogindata.h"

#include "models/sessionmodel.h"
#include "models/usersmodel.h"

#include <QApplication>
#include <QDir>

#include <KAuth/ExecuteJob>
#include <KIO/ApplicationLauncherJob>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KService>
#include <KUser>

K_PLUGIN_FACTORY_WITH_JSON(KCMPlasmaLoginFactory, "kcm_plasmalogin.json", registerPlugin<PlasmaLoginKcm>(); registerPlugin<PlasmaLoginData>();)

PlasmaLoginKcm::PlasmaLoginKcm(QObject *parent, const KPluginMetaData &metaData)
    : KQuickManagedConfigModule(parent, metaData)
    , m_data(new PlasmaLoginData(this))
{
    setAuthActionName(QStringLiteral("org.kde.kcontrol.kcmplasmalogin.save"));

    qmlRegisterType<UsersModel>("org.kde.private.kcms.plasmalogin", 1, 0, "UsersModel");
    qmlRegisterType<SessionModel>("org.kde.private.kcms.plasmalogin", 1, 0, "SessionModel");
    qmlRegisterAnonymousType<PlasmaLoginSettings>("org.kde.private.kcms.plasmalogin", 1);
}

PlasmaLoginKcm::~PlasmaLoginKcm()
{
}

PlasmaLoginSettings *PlasmaLoginKcm::plasmaLoginSettings() const
{
    return m_data->plasmaLoginSettings();
}

QString PlasmaLoginKcm::toLocalFile(const QUrl &url)
{
    return url.toLocalFile();
}

void PlasmaLoginKcm::save()
{
    QVariantMap args;
    args[QStringLiteral("kde_settings.conf/Autologin/User")] = m_data->plasmaLoginSettings()->user();
    args[QStringLiteral("kde_settings.conf/Autologin/Session")] = m_data->plasmaLoginSettings()->session();
    args[QStringLiteral("kde_settings.conf/Autologin/Relogin")] = m_data->plasmaLoginSettings()->relogin();
    args[QStringLiteral("kde_settings.conf/Users/MinimumUid")] = m_data->plasmaLoginSettings()->minimumUid();
    args[QStringLiteral("kde_settings.conf/Users/MaximumUid")] = m_data->plasmaLoginSettings()->maximumUid();
    args[QStringLiteral("kde_settings.conf/General/HaltCommand")] = m_data->plasmaLoginSettings()->haltCommand();
    args[QStringLiteral("kde_settings.conf/General/RebootCommand")] = m_data->plasmaLoginSettings()->rebootCommand();

    KAuth::Action saveAction(authActionName());
    saveAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));
    saveAction.setArguments(args);

    auto job = saveAction.execute();
    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            Q_EMIT errorOccured(job->errorString());
        } else {
            m_data->plasmaLoginSettings()->load();
        }
        // Clarify enable or disable the Apply button.
        this->setNeedsSave(job->error());
    });
    job->start();
}

void PlasmaLoginKcm::synchronizeSettings()
{
    // initial check for sddm user; abort if user not present
    // we have to check with QString and isEmpty() instead of QDir and exists() because
    // QDir returns "." and true for exists() in the case of a non-existent user;
    QString sddmHomeDirPath = KUser("sddm").homeDir();
    if (sddmHomeDirPath.isEmpty()) {
        Q_EMIT errorOccured(QString::fromUtf8(kli18n("Cannot proceed, user 'sddm' does not exist. Please check your SDDM install.").untranslatedText()));
        return;
    }

    // read Plasma values
    KConfig cursorConfig(QStringLiteral("kcminputrc"));
    KConfigGroup cursorConfigGroup(&cursorConfig, QStringLiteral("Mouse"));
    QString cursorTheme = cursorConfigGroup.readEntry("cursorTheme", QString());
    QString cursorSize = cursorConfigGroup.readEntry("cursorSize", QString());

    KConfig dpiConfig(QStringLiteral("kcmfonts"));
    KConfigGroup dpiConfigGroup(&dpiConfig, QStringLiteral("General"));
    QString dpiValue = dpiConfigGroup.readEntry("forceFontDPI");
    QString dpiArgument = QStringLiteral("-dpi ") + dpiValue;

    KConfig numLockConfig(QStringLiteral("kcminputrc"));
    KConfigGroup numLockConfigGroup(&numLockConfig, QStringLiteral("Keyboard"));
    QString numLock = numLockConfigGroup.readEntry("NumLock");

    // Syncing the font only works with SDDM >= 0.19, but will not have a negative effect with older versions
    KConfig plasmaFontConfig(QStringLiteral("kdeglobals"));
    KConfigGroup plasmaFontGroup(&plasmaFontConfig, QStringLiteral("General"));
    QString plasmaFont = plasmaFontGroup.readEntry("font", QApplication::font().toString());

    // define paths
    const QString fontconfigPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("fontconfig"), QStandardPaths::LocateDirectory);
    const QString kdeglobalsPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kdeglobals"));
    const QString plasmarcPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("plasmarc"));
    const QString kcminputrcPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kcminputrc"));
    const QString kwinoutputconfigPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("kwinoutputconfig.json"));

    // send values and paths to helper, debug if it fails
    QVariantMap args;

    args[QStringLiteral("kde_settings.conf")] = QString{QLatin1String(PLASMALOGIN_CONFIG_DIR "/") + QStringLiteral("kde_settings.conf")};

    args[QStringLiteral("sddm.conf")] = QLatin1String(PLASMALOGIN_CONFIG_FILE);

    if (!cursorTheme.isNull()) {
        args[QStringLiteral("kde_settings.conf/Theme/CursorTheme")] = cursorTheme;
    } else {
        qDebug() << "Cannot find cursor theme value; unsetting it";
        args[QStringLiteral("kde_settings.conf/Theme/CursorTheme")] = QVariant();
    }
    if (!cursorSize.isNull()) {
        args[QStringLiteral("kde_settings.conf/Theme/CursorSize")] = cursorSize;
    } else {
        qDebug() << "Cannot find cursor size value; unsetting it";
        args[QStringLiteral("kde_settings.conf/Theme/CursorSize")] = QVariant();
    }

    if (!dpiValue.isEmpty()) {
        args[QStringLiteral("kde_settings.conf/X11/ServerArguments")] = dpiArgument;
    } else {
        qDebug() << "Cannot find scaling DPI value.";
    }

    if (!numLock.isEmpty()) {
        if (numLock == QStringLiteral("0")) {
            args[QStringLiteral("kde_settings.conf/General/Numlock")] = QStringLiteral("on");
        } else if (numLock == QStringLiteral("1")) {
            args[QStringLiteral("kde_settings.conf/General/Numlock")] = QStringLiteral("off");
        } else if (numLock == QStringLiteral("2")) {
            args[QStringLiteral("kde_settings.conf/General/Numlock")] = QStringLiteral("none");
        }
    } else {
        qDebug() << "Cannot find NumLock value.";
    }

    if (!plasmaFont.isEmpty()) {
        args[QStringLiteral("kde_settings.conf/Theme/Font")] = plasmaFont;
    } else {
        qDebug() << "Cannot find Plasma font value.";
    }

    if (!fontconfigPath.isEmpty()) {
        args[QStringLiteral("fontconfig")] = fontconfigPath;
    } else {
        qDebug() << "Cannot find fontconfig folder.";
    }

    if (!kdeglobalsPath.isEmpty()) {
        args[QStringLiteral("kdeglobals")] = kdeglobalsPath;
    } else {
        qDebug() << "Cannot find kdeglobals file.";
    }

    if (!plasmarcPath.isEmpty()) {
        args[QStringLiteral("plasmarc")] = plasmarcPath;
    } else {
        qDebug() << "Cannot find plasmarc file.";
    }

    if (!kcminputrcPath.isEmpty()) {
        args[QStringLiteral("kcminputrc")] = kcminputrcPath;
    } else {
        qDebug() << "Cannot find kcminputrc file.";
    }

    if (!kwinoutputconfigPath.isEmpty()) {
        args[QStringLiteral("kwinoutputconfig")] = kwinoutputconfigPath;
    } else {
        qDebug() << "Cannot find kwinoutputconfiguration.json file";
    }

    auto path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kscreen/"), QStandardPaths::LocateDirectory);
    if (!path.isEmpty()) {
        args[QStringLiteral("kscreen-config")] = path;
    }

    // TODO: KAuth syncAction
    KAuth::Action syncAction(QStringLiteral("org.kde.kcontrol.kcmplasmaloginsync"));
    syncAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));
    syncAction.setArguments(args);

    auto job = syncAction.execute();
    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            qDebug() << "Synchronization failed";
            qDebug() << job->errorString();
            qDebug() << job->errorText();
            if (!job->errorText().isEmpty()) {
                Q_EMIT errorOccured(job->errorText());
            }
        } else {
            qDebug() << "Synchronization successful";
        }

        Q_EMIT syncAttempted();
    });
    job->start();
}

void PlasmaLoginKcm::resetSyncronizedSettings()
{
    // initial check for sddm user; abort if user not present
    // we have to check with QString and isEmpty() instead of QDir and exists() because
    // QDir returns "." and true for exists() in the case of a non-existent user
    QString sddmHomeDirPath = KUser("sddm").homeDir();
    if (sddmHomeDirPath.isEmpty()) {
        Q_EMIT errorOccured(QString::fromUtf8(kli18n("Cannot proceed, user 'sddm' does not exist. Please check your SDDM install.").untranslatedText()));
        return;
    }

    // send paths to helper
    QVariantMap args;

    args[QStringLiteral("kde_settings.conf")] = QStringLiteral(PLASMALOGIN_CONFIG_DIR "/kde_settings.conf");

    args[QStringLiteral("sddm.conf")] = QLatin1String(PLASMALOGIN_CONFIG_FILE);

    args[QStringLiteral("kde_settings.conf/Theme/CursorTheme")] = QVariant();

    args[QStringLiteral("kde_settings.conf/Theme/CursorSize")] = QVariant();

    args[QStringLiteral("kde_settings.conf/X11/ServerArguments")] = QVariant();

    args[QStringLiteral("kde_settings.conf/General/Numlock")] = QVariant();

    args[QStringLiteral("kde_settings.conf/Theme/Font")] = QVariant();

    args[QStringLiteral("theme.conf.user/General/showClock")] = true;

    // TODO: KAuth resetAction
    KAuth::Action resetAction(QStringLiteral("org.kde.kcontrol.kcmplasmalogin.reset"));
    resetAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmplasmalogin"));
    resetAction.setArguments(args);

    auto job = resetAction.execute();

    connect(job, &KJob::result, this, [this, job] {
        if (job->error()) {
            qDebug() << "Reset failed";
            qDebug() << job->errorString();
            qDebug() << job->errorText();
            if (!job->errorText().isEmpty()) {
                Q_EMIT errorOccured(job->errorText());
            }
        } else {
            qDebug() << "Reset successful";
        }

        Q_EMIT syncAttempted();
    });
    job->start();
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
