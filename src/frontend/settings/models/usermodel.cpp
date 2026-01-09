/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <KUser>

#include <pwd.h>

#include "plasmaloginsettings.h"

#include "usermodel.h"

UserModel::UserModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // TODO: Should use settings for uid limits, and repopulate on change
    populate();
}

int UserModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_users.count();
}

QVariant UserModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_users.count()) {
        return {};
    }

    User user = m_users[index.row()];

    switch (role) {
    case UserModel::NameRole:
        return user.name;
    case Qt::DisplayRole:
    case UserModel::RealNameRole:
        return user.realName;
    case UserModel::IconRole:
        return user.icon;
    case UserModel::HomeDirRole:
        return user.homeDir;
    case UserModel::NeedsPasswordRole:
        return user.needsPassword;
    case UserModel::UidRole:
        return user.uid;
    case UserModel::GidRole:
        return user.gid;
    }

    return {};
}

QHash<int, QByteArray> UserModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[NameRole] = "name";
    roles[RealNameRole] = "realName";
    roles[IconRole] = "icon";
    roles[HomeDirRole] = "homeDir";
    roles[NeedsPasswordRole] = "needsPassword";
    roles[UidRole] = "uid";
    roles[GidRole] = "gid";
    return roles;
}

int UserModel::indexOfData(const QVariant &data, int role) const
{
    if (data.isNull() || data.toString().isEmpty()) {
        return -1;
    }

    for (int i = 0; i < m_users.count(); ++i) {
        if (UserModel::data(index(i, 0), role) == data) {
            return i;
        }
    }

    return -1;
}

void UserModel::populate()
{
    beginResetModel();

    m_users.clear();

    for (const KUser &user : KUser::allUsers()) {
        if (!user.isValid()) {
            qWarning() << "Invalid user";
            continue;
        }

        const K_UID uid = user.userId().nativeId();
        const K_GID gid = user.groupId().nativeId();

        // Check between uid min/max
        if (uid < PlasmaLoginSettings::getInstance().minimumUid() || uid > PlasmaLoginSettings::getInstance().maximumUid()) {
            continue;
        }

        struct passwd *pw = getpwuid(uid);
        if (!pw) {
            qWarning() << "Failed to determine if user requires password for login";
            continue;
        }
        const bool needsPassword = strcmp(pw->pw_passwd, "") != 0;

        QString icon = user.faceIconPath();
        if (icon.isEmpty()) {
            icon = QStringLiteral("qrc:/qt/qml/org/kde/plasma/login/.face.icon");
        } else {
            icon.prepend(QStringLiteral("file://"));
        }

        m_users << User(user.loginName(), user.property(KUser::UserProperty::FullName).toString(), icon, user.homeDir(), needsPassword, uid, gid);
    }

    endResetModel();
}

#include "moc_usermodel.cpp"
