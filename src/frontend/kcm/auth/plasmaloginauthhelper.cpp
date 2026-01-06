/*
    SPDX-FileCopyrightText: 2019 Filip Fila <filipfila.kde@gmail.com>
    SPDX-FileCopyrightText: 2013 Reza Fatahilah Shah <rshah0385@kireihana.com>
    SPDX-FileCopyrightText: 2011, 2012 David Edmundson <kde@davidedmundson.co.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "plasmaloginauthhelper.h"
#include "config.h"

#include <unistd.h>

#include <QBuffer>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSharedPointer>

#include <KConfig>
#include <KConfigGroup>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KUser>

static const QFile::Permissions standardPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther;

/*
 * Return the plasmalogin user path
 */
static std::optional<QString> plasmaloginUserHomeDir()
{
    // we have to check with QString and isEmpty() instead of QDir and exists()
    // because QDir returns "." and true for exists() in the case of a
    // non-existent user
    const QString plasmaloginHomeDirPath = KUser("plasmalogin").homeDir();
    if (plasmaloginHomeDirPath.isEmpty()) {
        return std::nullopt;
    } else {
        return plasmaloginHomeDirPath;
    }
}

/*
 * Ensure correct ownership of the provided file or directory
 */
static void chownPath(const QString &path)
{
    static const KUser plasmaloginUser("plasmalogin");
    chown(path.toLocal8Bit().data(), plasmaloginUser.userId().nativeId(), plasmaloginUser.groupId().nativeId());
}

ActionReply PlasmaLoginAuthHelper::sync(const QVariantMap &args)
{
    QString homeDir;
    if (auto opt = plasmaloginUserHomeDir()) {
        homeDir = *opt;
    } else {
        return ActionReply::HelperErrorReply();
    }

    // In plasma-framework, ThemePrivate::useCache documents the requirement to
    // clear the cache when colors change while the app that uses them isn't
    // running; that condition applies to the greeter here, so clear the cache
    // if it exists to make sure plasma login has a fresh state
    QDir cacheLocation(homeDir + QStringLiteral("/.cache"));
    if (cacheLocation.exists()) {
        cacheLocation.removeRecursively();
    }

    QDir homeLocation(homeDir);

    // Create config location if it does not exist
    QDir configLocation(homeDir + QStringLiteral("/.config"));
    if (!configLocation.exists()) {
        homeLocation.mkdir(QStringLiteral(".config"), standardPermissions);
        chownPath(configLocation.path());
    }

    // Create fontconfig location if it does not exist
    QDir fontConfigLocation(homeDir + QStringLiteral("/.config/fontconfig"));
    if (!fontConfigLocation.exists()) {
        configLocation.mkdir(QStringLiteral("fontconfig"), standardPermissions);
        chownPath(fontConfigLocation.path());
    }

    auto createConfigFile = [&args, &homeDir](const QString &name) {
        // Don't create config for any file we weren't given - and remove any
        // existing config as it does not exist in the user's config folder
        if (!args.keys().contains(name)) {
            QFile(homeDir + QStringLiteral("/.config/")).remove();
            return;
        }

        const QString content = args.value(name).toString();
        QFile file(homeDir + QStringLiteral("/.config/") + name);
        if (file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate, standardPermissions)) {
            QTextStream out(&file);
            out << content;
            chownPath(file.fileName());
        }
    };

    createConfigFile(QStringLiteral("kdeglobals"));

    createConfigFile(QStringLiteral("plasmarc"));

    createConfigFile(QStringLiteral("kcminputrc"));

    createConfigFile(QStringLiteral("kwinoutputconfig.json"));

    createConfigFile(QStringLiteral("fontconfig/fonts.conf"));

    return ActionReply::SuccessReply();
}

ActionReply PlasmaLoginAuthHelper::reset(const QVariantMap &args)
{
    Q_UNUSED(args);

    QString homeDir;
    if (auto opt = plasmaloginUserHomeDir()) {
        homeDir = *opt;
    } else {
        return ActionReply::HelperErrorReply();
    }

    QDir cacheLocation(homeDir + QStringLiteral("/.cache"));
    if (cacheLocation.exists()) {
        cacheLocation.removeRecursively();
    }

    QDir fontConfigDir(homeDir + QStringLiteral("/.config/fontconfig"));
    if (fontConfigDir.exists()) {
        fontConfigDir.removeRecursively();
    }

    QFile kdeglobalsFile(homeDir + QStringLiteral("/.config/") + QStringLiteral("kdeglobals"));
    kdeglobalsFile.remove();

    QFile plasmarcFile(homeDir + QStringLiteral("/.config/") + QStringLiteral("/plasmarc"));
    plasmarcFile.remove();

    QFile kcminputrcFile(homeDir + QStringLiteral("/.config/") + QStringLiteral("/kcminputrc"));
    kcminputrcFile.remove();

    QFile kwinoutputconfigFile(homeDir + QStringLiteral("/.config/") + QStringLiteral("/kwinoutputconfig.json"));
    kwinoutputconfigFile.remove();

    return ActionReply::SuccessReply();
}

ActionReply PlasmaLoginAuthHelper::save(const QVariantMap &args)
{
    qDebug() << "SAVE";
    // main config (why is this in /etc?)
    {
        QFile file(QLatin1String(PLASMALOGIN_CONFIG_FILE));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate, standardPermissions)) {
            return ActionReply::HelperErrorReply();
        }

        QTextStream out(&file);
        out << args[QStringLiteral("config")].toString();
        out.flush();
        file.close();

               // Ensure permissions on the config file are appropriate
        if (file.permissions() != standardPermissions) {
            file.setPermissions(standardPermissions);
        }
    }

    // wallpaper
    if (args.contains("wallpaper"))
    {
        QString homeDir;
        if (auto opt = plasmaloginUserHomeDir()) {
            homeDir = *opt;
        } else {
            return ActionReply::HelperErrorReply();
        }
        QFile file(homeDir + "/wallpaper.png");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate, standardPermissions)) {
            return ActionReply::HelperErrorReply();
        }
        QDataStream out(&file);
        out << args["wallpaper"].toByteArray();

        qDebug() << "written wallapper";
        qDebug() << args["wallpaperFd"].value<QDBusUnixFileDescriptor>().isValid();
    }

    return ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplasmalogin", PlasmaLoginAuthHelper)

#include "moc_plasmaloginauthhelper.cpp"
