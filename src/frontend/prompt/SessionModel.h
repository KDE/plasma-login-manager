/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QAbstractListModel>
#include <QUrl>

struct Session {
    enum Type {
        X11,
        Wayland
    };

    Type type;
    QString path;
    QString displayName;
    QString comment;

    Session(Type type, QString path, QString displayName, QString comment)
        : type(type)
        , path(std::move(path))
        , displayName(std::move(displayName))
        , comment(std::move(comment))
    {
    }
};

class SessionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    SessionModel(QObject *parent = nullptr);
    ~SessionModel() override = default;

    enum SessionRoles {
        TypeRole = Qt::UserRole + 1,
        PathRole,
        FileNameRole,
        DisplayNameRole,
        CommentRole
    };
    Q_ENUM(SessionRoles)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    void repopulate(const QStringList &xSessionPaths, const QStringList &waylandSessionPaths);
    void addSession(const QString path, const Session::Type type);

    QList<Session> m_sessions;
};
