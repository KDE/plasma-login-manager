/*
    SPDX-FileCopyrightText: 2019 Filip Fila <filipfila.kde@gmail.com>
    SPDX-FileCopyrightText: 2013 Reza Fatahilah Shah <rshah0385@kireihana.com>
    SPDX-FileCopyrightText: 2011, 2012 David Edmundson <kde@davidedmundson.co.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "plasmaloginauthhelper.h"
#include "config.h"

#include <dirent.h>
#include <fcntl.h> /* Definition of O_* and S_* constants */
#include <linux/openat2.h> /* Definition of RESOLVE_* constants */
#include <sys/stat.h>
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

template<typename Func>
static bool runAsPlasmaLoginUser(Func function)
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
        if (waitpid(pid, &status, 0) < 0) {
            return false;
        }
        return status == EXIT_SUCCESS;
    }
}

static bool adjustOwnershipRecursively(int dirFd, uid_t uid, gid_t gid, const QString &path)
{
    bool success = true;

    DIR *dir = fdopendir(dirFd);
    if (!dir) {
        qWarning() << "Could not open directory stream for" << path << ":" << strerror(errno);
        close(dirFd);
        return false;
    }
    auto closeDir = qScopeGuard([dir]() {
        closedir(dir);
    });

    while (dirent *entry = readdir(dir)) {
        const QByteArray name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }

        struct stat statBuf;
        if (fstatat(dirFd, name.constData(), &statBuf, AT_SYMLINK_NOFOLLOW) != 0) {
            qWarning() << "Could not stat" << path + QLatin1Char('/') + QString::fromUtf8(name) << ":" << strerror(errno);
            success = false;
            continue;
        }

        if (S_ISLNK(statBuf.st_mode)) {
            continue;
        }

        if (S_ISDIR(statBuf.st_mode)) {
            struct open_how how = {.flags = O_RDONLY | O_DIRECTORY | O_CLOEXEC,
                                   .mode = 0,
                                   .resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS};
            const int childFd = syscall(SYS_openat2, dirFd, name.constData(), &how, sizeof(struct open_how));
            if (childFd < 0) {
                qWarning() << "Could not open directory" << path + QLatin1Char('/') + QString::fromUtf8(name) << ":" << strerror(errno);
                success = false;
                continue;
            }

            success = adjustOwnershipRecursively(childFd, uid, gid, path + QLatin1Char('/') + QString::fromUtf8(name)) && success;
        }

        if (fchownat(dirFd, name.constData(), uid, gid, AT_SYMLINK_NOFOLLOW) != 0) {
            qWarning() << "Could not change owner of" << path + QLatin1Char('/') + QString::fromUtf8(name) << ":" << strerror(errno);
            success = false;
        }
    }

    return success;
}

bool PlasmaLoginAuthHelper::adjustPermissionsFromPlasma6_6()
{
    // Plasma 6.6 ran some things as root. For 6.7 onwards we run them as the
    // plasmalogin user, so upgraded installations may need ownership repaired.
    KUser user(QStringLiteral("plasmalogin"));
    if (!user.isValid()) {
        qWarning() << "Could not find plasmalogin user";
        return false;
    }

    const QString homeDirPath = user.homeDir();
    if (homeDirPath.isEmpty()) {
        qWarning() << "Could not determine home directory for plasmalogin user";
        return false;
    }

    const QByteArray homeDirPathUtf8 = homeDirPath.toUtf8();
    const int homeDirFd = open(homeDirPathUtf8.constData(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (homeDirFd < 0) {
        qWarning() << "Could not open home directory for plasmalogin user:" << strerror(errno);
        return false;
    }

    return adjustOwnershipRecursively(homeDirFd, user.userId().nativeId(), user.groupId().nativeId(), homeDirPath);
}

ActionReply PlasmaLoginAuthHelper::sync(const QVariantMap &args)
{
    QString homeDir;
    if (auto opt = plasmaloginUserHomeDir()) {
        homeDir = *opt;
    } else {
        return ActionReply::HelperErrorReply();
    }

    if (!adjustPermissionsFromPlasma6_6()) {
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

        createConfigFile(QStringLiteral("kxkbrc"));

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

    if (!adjustPermissionsFromPlasma6_6()) {
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

static bool isRegularFd(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        return false;
    }
    if (flags & O_PATH) {
        return false;
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

ActionReply PlasmaLoginAuthHelper::save(const QVariantMap &args)
{
    QFile file(QLatin1String(PLASMALOGIN_CONFIG_FILE));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate, standardPermissions)) {
        return ActionReply::HelperErrorReply();
    }

    if (args[QStringLiteral("config")].toString().toUtf8().size() > 1 * 1024 * 1024) {
        qWarning() << "Config size is greater than 1 MiB";
        return ActionReply::HelperErrorReply();
    } else {
        QTextStream out(&file);
        out << args[QStringLiteral("config")].toString();
        out.flush();
    }

    file.close();

    // Ensure permissions on the config file are appropriate
    if (file.permissions() != standardPermissions) {
        file.setPermissions(standardPermissions);
    }

    // Wallpaper stuff
    // This is in /var/plasmalogin so we drop privileges
    QString homeDirPath;
    if (auto opt = plasmaloginUserHomeDir()) {
        homeDirPath = *opt;
    } else {
        qWarning() << "Could not determine home directory for plasmalogin user";
        return ActionReply::HelperErrorReply();
    }

    if (!adjustPermissionsFromPlasma6_6()) {
        return ActionReply::HelperErrorReply();
    }

    bool rc = runAsPlasmaLoginUser([homeDirPath, args]() {
        QDir homeDir(homeDirPath);
        QDir wallpaperDir(homeDir.absoluteFilePath("wallpapers"));
        if (!wallpaperDir.removeRecursively()) {
            qWarning() << "Could not clean old wallpaper directory";
        }
        homeDir.mkdir("wallpapers");

        auto rootWallpaperFd = open(wallpaperDir.path().toUtf8().constData(), O_RDONLY | O_DIRECTORY);
        if (rootWallpaperFd < 0) {
            qWarning() << "Could not load root wallpaper directory." << qPrintable(strerror(errno));
            return false;
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
                return false;
            }

            const QString relativeFilePath = "wallpapers/" + wallpaper;
            const QString relativeParentDirectory = relativeFilePath.left(relativeFilePath.lastIndexOf("/"));
            if (!homeDir.mkpath(relativeParentDirectory)) {
                qWarning() << "Could not create new wallpaper directory";
                return false;
            }

            struct open_how how = {.flags = O_CREAT | O_WRONLY | O_TRUNC,
                                   .mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                                   .resolve = RESOLVE_BENEATH | RESOLVE_NO_MAGICLINKS};
            int outFd = syscall(SYS_openat2, rootWallpaperFd, wallpaper.toUtf8().constData(), &how, sizeof(struct open_how));
            if (outFd < 0) {
                qWarning() << "Could not open wallpaper file." << qPrintable(strerror(errno));
                return false;
            }
            QFile file;
            if (!file.open(outFd, QIODevice::WriteOnly | QIODevice::Truncate, QFileDevice::AutoCloseHandle)) {
                qWarning() << "Could not open wallpaper directory from FD.";
                return false;
            }

            QDataStream out(&file);
            QDBusUnixFileDescriptor fd = args.value("_fd_" + wallpaper).value<QDBusUnixFileDescriptor>();
            if (!fd.isValid() || !isRegularFd(fd.fileDescriptor())) {
                qWarning() << "Could not retrieve wallpaper" << wallpaper;
                continue;
            }
            QFile wallpaperIn;
            if (!wallpaperIn.open(fd.fileDescriptor(), QIODevice::ReadOnly)) {
                qWarning() << "Failed to open wallpaper";
                return false;
            }
            QByteArray buf(4096, 0);
            while (true) {
                qint64 n = wallpaperIn.read(buf.data(), buf.size());
                if (n == 0) {
                    break;
                } else if (n < 0) {
                    qWarning() << "Failed to transfer wallpaper data for file" << relativeFilePath;
                    return false;
                } else {
                    out.writeRawData(buf.data(), n);
                }
            }
        }
        return true;
    });

    if (rc) {
        return ActionReply::SuccessReply();
    } else {
        return ActionReply::HelperErrorReply();
    }
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmplasmalogin", PlasmaLoginAuthHelper)

#include "moc_plasmaloginauthhelper.cpp"
