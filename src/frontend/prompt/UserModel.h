/*
 *  This file originates from SDDM.
 *
 *  SPDX-FileCopyrightText: 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>

#include <QHash>

namespace PlasmaLogin
{
class UserModelPrivate;

class UserModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(UserModel)

public:
    UserModel(bool needAllUsers, QObject *parent = 0);
    ~UserModel();

    enum UserRoles {
        NameRole = Qt::UserRole + 1,
        RealNameRole,
        HomeDirRole,
        IconRole,
        NeedsPasswordRole
    };
    Q_ENUM(UserRoles)

    Q_PROPERTY(int lastIndex READ lastIndex CONSTANT)
    Q_PROPERTY(QString lastUser READ lastUser CONSTANT)
    Q_PROPERTY(int count READ rowCount CONSTANT)
    Q_PROPERTY(int disableAvatarsThreshold READ disableAvatarsThreshold CONSTANT)
    Q_PROPERTY(bool containsAllUsers READ containsAllUsers CONSTANT)

    QHash<int, QByteArray> roleNames() const override;

    int lastIndex() const;
    QString lastUser() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int disableAvatarsThreshold() const;
    bool containsAllUsers() const;

private:
    UserModelPrivate *d{nullptr};
};
}
