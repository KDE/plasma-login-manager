/*
    SPDX-FileCopyrightText: 2019 Filip Fila <filipfila.kde@gmail.com>
    SPDX-FileCopyrightText: 2013 Reza Fatahilah Shah <rshah0385@kireihana.com>
    SPDX-FileCopyrightText: 2011, 2012 David Edmundson <kde@davidedmundson.co.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "plasmaloginauthhelper.h"
#include "config.h"

#include <fcntl.h> /* Definition of O_* and S_* constants */
#include <linux/openat2.h> /* Definition of RESOLVE_* constants */
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <sys/wait.h>
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
static const QFile::Permissions standardDirectoryPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther;

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

static bool runAsPlasmaLoginUser(std::function<bool()> function)
{
    KUser user("plasmalogin");
    if (!user.isValid()) {
        qWarning() << "Could not find plasmalogin user";
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        qWarning() << "Failed to fork plasmalogin helper process";
        return false;
    } else if (pid == 0) {
        if (setgid(user.groupId().nativeId()) != 0) {
            qWarning() << "Failed to setgid in plasmalogin helper subprocess";
            _exit(EXIT_FAILURE);
        }
        if (setuid(user.userId().nativeId()) != 0) {
            qWarning() << "Failed to setuid in plasmalogin helper subprocess";
            _exit(EXIT_FAILURE);
        }

        if (function()) {
            _exit(EXIT_SUCCESS);
        } else {
            _exit(EXIT_FAILURE);
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        return status == EXIT_SUCCESS;
    }
}

ActionReply PlasmaLoginAuthHelper::sync(const QVariantMap &args)
{
    QString homeDir;
    if (auto opt = plasmaloginUserHomeDir()) {
        homeDir = *opt;
    } else {
        return ActionReply::HelperErrorReply();
    }

    bool rc = runAsPlasmaLoginUser([args, homeDir]() {
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
            homeLocation.mkdir(QStringLiteral(".config"), standardDirectoryPermissions);
        }

        // Create fontconfig location if it does not exist
        QDir fontConfigLocation(homeDir + QStringLiteral("/.config/fontconfig"));
        if (!fontConfigLocation.exists()) {
            configLocation.mkdir(QStringLiteral("fontconfig"), standardDirectoryPermissions);
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
            }
        };

        createConfigFile(QStringLiteral("kdeglobals"));

        createConfigFile(QStringLiteral("plasmarc"));

        createConfigFile(QStringLiteral("kcminputrc"));

        createConfigFile(QStringLiteral("kwinoutputconfig.json"));

        createConfigFile(QStringLiteral("fontconfig/fonts.conf"));
        return true;
    });

    if (rc) {
        return ActionReply::SuccessReply();
    } else {
        return ActionReply::HelperErrorReply();
    }
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

    bool rc = runAsPlasmaLoginUser([homeDir]() {
        QDir cacheDir(homeDir + QStringLiteral("/.cache"));
        if (cacheDir.exists()) {
            cacheDir.removeRecursively();
        }

        // Remove all config, including what we've synced, but
        // also anything potentially created by running things
        QDir configDir(homeDir + QStringLiteral("/.config"));
        if (configDir.exists()) {
            configDir.removeRecursively();
        }

        return true;
    });

    if (rc) {
        return ActionReply::SuccessReply();
    } else {
        return ActionReply::HelperErrorReply();
    }
}

ActionReply PlasmaLoginAuthHelper::save(const QVariantMap &args)
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

    // wallpaper stuff
    QString homeDirPath;
    if (auto opt = plasmaloginUserHomeDir()) {
        homeDirPath = *opt;
    } else {
        qWarning() << "Could not determine home directory for plasmalogin user";
        return ActionReply::HelperErrorReply();
    }


    QDir homeDir(homeDirPath);
    QDir wallpaperDir(homeDir.absoluteFilePath("wallpapers"));
    if (!wallpaperDir.removeRecursively()) {
        qWarning() << "Could not clean old wallpaper directory";
    }
    homeDir.mkdir("wallpapers");

    auto rootWallpaperFd = open(wallpaperDir.path().toUtf8().constData(), O_RDONLY | O_DIRECTORY);
    if (rootWallpaperFd < 0) {
        qWarning() << "Could not load root wallpaper directory." << qPrintable(strerror(errno));
        return ActionReply::HelperErrorReply();
    }
    auto closeRootWallpaperFd = qScopeGuard([&]() {
        close(rootWallpaperFd);
    });

    const QStringList wallpapers = args[QStringLiteral("wallpapers")].toStringList();
    for (const QString &wallpaper : wallpapers) {
        // This shouldn't be needed with the explicit openat flags, but
        // another check can't hurt
        if (wallpaper.contains("..")) {
            qWarning() << "Badly formed wallpaper name detected, aborting";
            return ActionReply::HelperErrorReply();
        }

        const QString relativeFilePath = "wallpapers/" + wallpaper;
        const QString relativeParentDirectory = relativeFilePath.left(relativeFilePath.lastIndexOf("/"));
        if (!homeDir.mkpath(relativeParentDirectory)) {
            qWarning() << "Could not create new wallpaper directory";
            return ActionReply::HelperErrorReply();
        }

         struct open_how how = {
            .flags = O_CREAT | O_WRONLY | O_TRUNC,
            .mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
            .resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS
        };
        int outFd = syscall(SYS_openat2, rootWallpaperFd, wallpaper.toUtf8().constData(), &how, sizeof(struct open_how));        
        if (outFd < 0) {
            qWarning() << "Could not open wallpaper file." << qPrintable(strerror(errno));
            return ActionReply::HelperErrorReply();
        }
        QFile file;
        if (!file.open(outFd, QIODevice::WriteOnly | QIODevice::Truncate, QFileDevice::AutoCloseHandle)) {
            qWarning() << "Could not open wallpaper directory from FD.";
            return ActionReply::HelperErrorReply();
        }

        QDataStream out(&file);
        QDBusUnixFileDescriptor fd = args.value("_fd_" + wallpaper).value<QDBusUnixFileDescriptor>();
        if (!fd.isValid()) {
            qWarning() << "Could not retrieve wallpaper" << wallpaper;
            continue;
        }
        QFile wallpaperIn;
        if (!wallpaperIn.open(fd.fileDescriptor(), QIODevice::ReadOnly)) {
            qWarning() << "Failed to open wallpaper";
            return ActionReply::HelperErrorReply();
        }
        QByteArray buf(4096, 0);
        while (true) {
            qint64 n = wallpaperIn.read(buf.data(), buf.size());
            if (n == 0) {
                break;
            } else if (n < 0) {
                qWarning() << "Failed to transfer wallpaper data for file" << relativeFilePath;
                return ActionReply::HelperErrorReply();
            } else {
                out.writeRawData(buf.data(), n);
            }
        }
    }

    return ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplasmalogin", PlasmaLoginAuthHelper)

#include "moc_plasmaloginauthhelper.cpp"
