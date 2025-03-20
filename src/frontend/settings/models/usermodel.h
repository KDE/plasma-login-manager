/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

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
        GidRole
    };
    Q_ENUM(UserRoles)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    void populate();

    QList<User> m_users;
};
