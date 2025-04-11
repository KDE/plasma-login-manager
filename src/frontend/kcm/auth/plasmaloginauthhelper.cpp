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

static QSharedPointer<KConfig> openConfig(const QString &filePath)
{
    // if the plasmalogin.conf.d folder doesn't exist we fail to set the right permissions for kde_settings.conf
    QFileInfo fileLocation(filePath);
    QDir dir(fileLocation.absolutePath());
    if (!dir.exists()) {
        QDir().mkpath(dir.path());
    }
    QFile file(filePath);
    if (!file.exists()) {
        // If we are creating the config file, ensure it is world-readable: if
        // we don't do that, KConfig will create a file which is only readable
        // by root
        file.open(QIODevice::WriteOnly);
        file.close();
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
    }
    // in case the file has already been created with wrong permissions
    else if (!(file.permissions() & QFile::ReadOwner & QFile::WriteOwner & QFile::ReadGroup & QFile::ReadOther)) {
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
    }

    return QSharedPointer<KConfig>(new KConfig(file.fileName(), KConfig::SimpleConfig));
}

static QString PlasmaLoginUserCheck()
{
    // check for plasmalogin user; return empty string if user not present
    // we have to check with QString and isEmpty() instead of QDir and exists() because
    // QDir returns "." and true for exists() in the case of a non-existent user;
    const QString plasmaloginHomeDirPath = KUser("plasmalogin").homeDir();
    if (plasmaloginHomeDirPath.isEmpty()) {
        qDebug() << "Cannot proceed, user 'plasmalogin' does not exist. Please check your PLASMALOGIN install.";
        return QString();
    } else {
        return plasmaloginHomeDirPath;
    }
}

void PlasmaLoginAuthHelper::copyDirectoryRecursively(const QString &source, const QString &destination, QSet<QString> &done)
{
    if (done.contains(source)) {
        return;
    }
    done.insert(source);

    const QDir sourceDir(source);
    const auto entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    for (const auto &entry : entries) {
        const QString destinationPath = destination + QLatin1Char('/') + entry.fileName();
        if (entry.isFile()) {
            copyFile(entry.absoluteFilePath(), destinationPath);
        } else {
            QDir().mkpath(destinationPath);
            copyDirectoryRecursively(entry.absoluteFilePath(), destinationPath, done);
        }
    }
}

void PlasmaLoginAuthHelper::copyFile(const QString &source, const QString &destination)
{
    KUser plasmaloginUser(QStringLiteral("plasmalogin"));

    if (QFile::exists(destination)) {
        QFile::remove(destination);
    }

    if (!QFile::copy(source, destination)) {
        qWarning() << "Could not copy" << source << "to" << destination;
    }
    const char *destinationConverted = destination.toLocal8Bit().data();
    if (chown(destinationConverted, plasmaloginUser.userId().nativeId(), plasmaloginUser.groupId().nativeId())) {
        return;
    }
}

ActionReply PlasmaLoginAuthHelper::sync(const QVariantMap &args)
{
    Q_UNUSED(args);
    return ActionReply::HelperErrorReply();


    // abort if user not present
    const QString plasmaloginHomeDirPath = PlasmaLoginUserCheck();
    if (plasmaloginHomeDirPath.isEmpty()) {
        return ActionReply::HelperErrorReply();
    }

    // In libplasma, ThemePrivate::useCache documents the requirement to
    // clear the cache when colors change while the app that uses them isn't running;
    // that condition applies to the PLASMALOGIN greeter here, so clear the cache if it
    // exists to make sure PLASMALOGIN has a fresh state
    QDir plasmaloginCacheLocation(plasmaloginHomeDirPath + QStringLiteral("/.cache"));
    if (plasmaloginCacheLocation.exists()) {
        plasmaloginCacheLocation.removeRecursively();
    }

    // create PlasmaLogin config directory if it does not exist
    QDir plasmaloginConfigLocation(plasmaloginHomeDirPath + QStringLiteral("/.config"));
    if (!plasmaloginConfigLocation.exists()) {
        QDir().mkpath(plasmaloginConfigLocation.path());
    }

    // copy fontconfig (font, font rendering)
    if (!args[QStringLiteral("fontconfig")].isNull()) {
        QDir fontconfigSource(args[QStringLiteral("fontconfig")].toString());
        QStringList sourceFileEntries = fontconfigSource.entryList(QDir::Files);
        QStringList sourceDirEntries = fontconfigSource.entryList(QDir::AllDirs);
        QDir fontconfigDestination(plasmaloginConfigLocation.path() + QStringLiteral("/fontconfig"));

        if (!fontconfigDestination.exists()) {
            QDir().mkpath(fontconfigDestination.path());
        }

        if (sourceDirEntries.count() != 0) {
            for (int i = 0; i < sourceDirEntries.count(); i++) {
                QString directoriesSource = fontconfigSource.path() + QDir::separator() + sourceDirEntries[i];
                QString directoriesDestination = fontconfigDestination.path() + QDir::separator() + sourceDirEntries[i];
                fontconfigSource.mkpath(directoriesDestination);
                copyFile(directoriesSource, directoriesDestination);
            }
        }

        if (sourceFileEntries.count() != 0) {
            for (int i = 0; i < sourceFileEntries.count(); i++) {
                QString filesSource = fontconfigSource.path() + QDir::separator() + sourceFileEntries[i];
                QString filesDestination = fontconfigDestination.path() + QDir::separator() + sourceFileEntries[i];
                copyFile(filesSource, filesDestination);
            }
        }
    }

    // copy kdeglobals (color scheme)
    if (!args[QStringLiteral("kdeglobals")].isNull()) {
        QDir kdeglobalsSource(args[QStringLiteral("kdeglobals")].toString());
        QDir kdeglobalsDestination(plasmaloginConfigLocation.path() + QStringLiteral("/kdeglobals"));
        copyFile(kdeglobalsSource.path(), kdeglobalsDestination.path());
    }

    // copy plasmarc (icons, UI style)
    if (!args[QStringLiteral("plasmarc")].isNull()) {
        QDir plasmarcSource(args[QStringLiteral("plasmarc")].toString());
        QDir plasmarcDestination(plasmaloginConfigLocation.path() + QStringLiteral("/plasmarc"));
        copyFile(plasmarcSource.path(), plasmarcDestination.path());
    }
    if (!args[QStringLiteral("kcminputrc")].isNull()) {
        QDir kcminputrcSource(args[QStringLiteral("kcminputrc")].toString());
        QDir kcminputrcDestination(plasmaloginConfigLocation.path() + QStringLiteral("/kcminputrc"));
        copyFile(kcminputrcSource.path(), kcminputrcDestination.path());
    }

    // copy KWin config
    if (!args[QStringLiteral("kwinoutputconfig")].isNull()) {
        QDir source(args[QStringLiteral("kwinoutputconfig")].toString());
        QDir destination(plasmaloginConfigLocation.path() + QStringLiteral("/kwinoutputconfig.json"));
        copyFile(source.path(), destination.path());
    }

    return ActionReply::SuccessReply();
}

ActionReply PlasmaLoginAuthHelper::reset(const QVariantMap &args)
{
    Q_UNUSED(args);
    return ActionReply::HelperErrorReply();


    // abort if user not present
    const QString plasmaloginHomeDirPath = PlasmaLoginUserCheck();
    if (plasmaloginHomeDirPath.isEmpty()) {
        return ActionReply::HelperErrorReply();
    }

    QDir plasmaloginConfigLocation(plasmaloginHomeDirPath + QStringLiteral("/.config"));
    QDir fontconfigDir(args[QStringLiteral("plasmaloginUserConfig")].toString() + QStringLiteral("/fontconfig"));

    fontconfigDir.removeRecursively();
    QFile::remove(plasmaloginConfigLocation.path() + QStringLiteral("/kdeglobals"));
    QFile::remove(plasmaloginConfigLocation.path() + QStringLiteral("/plasmarc"));
    QFile::remove(plasmaloginConfigLocation.path() + QStringLiteral("/kcminputrc"));

    return ActionReply::SuccessReply();

}

ActionReply PlasmaLoginAuthHelper::save(const QVariantMap &args)
{
    QFile file(QLatin1String(PLASMALOGIN_CONFIG_FILE));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return ActionReply::HelperErrorReply();
    }

    QTextStream out(&file);
    out << args[QStringLiteral("config")].toString();
    out.flush();
    file.close();

    // Ensure permissions on the config file are appropriate
    if (!(file.permissions() & QFile::ReadOwner & QFile::WriteOwner & QFile::ReadGroup & QFile::ReadOther)) {
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
    }

    return ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplasmalogin", PlasmaLoginAuthHelper)

#include "moc_plasmaloginauthhelper.cpp"
