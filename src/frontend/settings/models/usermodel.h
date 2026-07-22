/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractListModel>
#include <QUrl>

struct User {
    QString name;
    QString realName;
    QString icon;
    QString homeDir;
    bool needsPassword;
    int uid;
    int gid;

    User(QString name, QString realName, QString icon, QString homeDir, bool needsPassword, int uid, int gid)
        : name(std::move(name))
        , realName(std::move(realName))
        , icon(std::move(icon))
        , homeDir(std::move(homeDir))
        , needsPassword(needsPassword)
        , uid(uid)
        , gid(gid)
    {
    }
};

class UserModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount CONSTANT)

public:
    UserModel(QObject *parent = nullptr);
    ~UserModel() override = default;

    enum UserRoles {
        NameRole = Qt::UserRole + 1,
        RealNameRole,
        IconRole,
        HomeDirRole,
        NeedsPasswordRole,
        UidRole,
        GidRole,
        ExistingSessionTypeRole,
    };
    Q_ENUM(UserRoles)

    enum ExistingSessionType {
        // User is not logged into a graphical session anywhere
        NoGraphicalSession,
        // User is logged into a graphical session on the same seat, allow login to resume that session
        ResumableGraphicalSession,
        // User is logged into a graphical session on another seat, login should be denied to prevent multiple graphical sessions
        BlockingGraphicalSession
    };
    Q_ENUM(ExistingSessionType)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int indexOfData(const QVariant &data, int role = Qt::DisplayRole) const;

private:
    void populate();

    ExistingSessionType existingSessionTypeForUser(const QString &name);

    QList<User> m_users;
};
