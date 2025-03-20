/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardPaths>

#include <KDesktopFile>
#include <KLocalizedString>

#include "SessionModel.h"

SessionModel::SessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // NOTE: /usr/local/share is listed first, then /usr/share, so sessions in the former take precedence
    const QStringList xSessionPaths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "xsessions", QStandardPaths::LocateDirectory);
    const QStringList waylandSessionPaths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "wayland-sessions", QStandardPaths::LocateDirectory);

    // NOTE: SDDM checks for the existence of /dev/dri before including wayland sessions
    // This is not duplicated here — if wayland isn't going to work, then neither is the greeter

    repopulate(xSessionPaths, waylandSessionPaths);

    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPaths(xSessionPaths + waylandSessionPaths);
    connect(watcher, &QFileSystemWatcher::directoryChanged, [this, xSessionPaths, waylandSessionPaths]() {
        repopulate(xSessionPaths, waylandSessionPaths);
    });
}

int SessionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_sessions.count();
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_sessions.count()) {
        return {};
    }

    Session session = m_sessions[index.row()];

    auto getDisplay = [this, session]() {
        // Here we want to handle gracefully any sessions with the same display name by disambiguating using
        // the session type and if not enough, an index (which will be as consistent as the installed files)

        const bool shouldAppendType = std::any_of(m_sessions.cbegin(), m_sessions.cend(), [session](const Session &other) {
            return session.path != other.path // Don't compare to ourselves
                && session.displayName == other.displayName // Display names are the same...
                && session.type != other.type; // ...but the type is different
        });

        int index = 1;
        const bool shouldAppendIndex = std::any_of(m_sessions.cbegin(), m_sessions.cend(), [session, &index](const Session &other) {
            const bool match = session.path != other.path // Don't compare to ourselves
                && session.displayName == other.displayName // Display names are the same...
                && session.type == other.type; // ..and so is the type

            if (match && other.path < session.path) {
                ++index;
            }

            return match;
        });

        if (shouldAppendType && shouldAppendIndex) {
            switch (session.type) {
            case Session::Type::X11:
                return i18nc("@item:inmenu %1 is the localised name of a desktop session, %2 is the index of the session",
                             "%1 (X11) (%2)",
                             session.displayName,
                             index);
            case Session::Type::Wayland:
                return i18nc("@item:inmenu %1 is the localised name of a desktop session, %2 is the index of the session",
                             "%1 (Wayland) (%2)",
                             session.displayName,
                             index);
            }
        } else if (shouldAppendType) {
            switch (session.type) {
            case Session::Type::X11:
                return i18nc("@item:inmenu %1 is the localised name of a desktop session", "%1 (X11)", session.displayName);
            case Session::Type::Wayland:
                return i18nc("@item:inmenu %1 is the localised name of a desktop session", "%1 (Wayland)", session.displayName);
            }
        } else if (shouldAppendIndex) {
            return i18nc("@item:inmenu %1 is the localised name of a desktop session, %2 is the index of the sessionn", "%1 (%2)", session.displayName, index);
        }

        return session.displayName;
    };

    switch (role) {
    case Qt::DisplayRole:
        return getDisplay();
    case SessionModel::DisplayNameRole:
        return session.displayName;
    case SessionModel::TypeRole:
        return session.type;
    case SessionModel::PathRole:
        return session.path;
    case SessionModel::FileNameRole:
        return QFileInfo(session.path).fileName();
    case SessionModel::CommentRole:
        return session.comment;
    default:
        break;
    }

    return {};
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[TypeRole] = "type";
    roles[PathRole] = "path";
    roles[DisplayNameRole] = "displayName";
    roles[CommentRole] = "comment";
    return roles;
}

void SessionModel::repopulate(const QStringList &xSessionPaths, const QStringList &waylandSessionPaths)
{
    beginResetModel();

    m_sessions.clear();

    auto findSessions = [](const QStringList &sessionPaths) {
        QStringList sessions;

        for (const auto &sessionPath : sessionPaths) {
            QDir dir = sessionPath;
            dir.setNameFilters({QStringLiteral("*.desktop")});
            dir.setFilter(QDir::Files);

            for (const auto &session : dir.entryList()) {
                QString sessionFileName = QFileInfo(session).fileName();

                // Ignore duplicate sessions, already added ones take precedence
                bool isDuplicate = false;
                for (const auto &existingSession : sessions) {
                    if (QFileInfo(existingSession).fileName() == sessionFileName) {
                        isDuplicate = true;
                        break;
                    }
                }

                if (!isDuplicate) {
                    sessions.append(sessionPath + "/" + session);
                }
            }
        }

        return sessions;
    };

    for (const auto &xSession : findSessions(xSessionPaths)) {
        addSession(xSession, Session::Type::X11);
    }

    for (const auto &waylandSession : findSessions(waylandSessionPaths)) {
        addSession(waylandSession, Session::Type::Wayland);
    }

    /*
    // Useful for testing SessionModel::Data(index, Qt::DisplayRole) getDisplay lambda
    m_sessions << Session(Session::Type::Wayland, "/foo/bar1.desktop", "Plasma", "This is Plasma Wayland 1");
    m_sessions << Session(Session::Type::Wayland, "/foo/bar2.desktop", "Plasma", "This is Plasma Wayland 2");
    m_sessions << Session(Session::Type::X11, "/foo/bar3.desktop", "Plasma", "This is Plasma X11 1");
    */

    endResetModel();
}

void SessionModel::addSession(const QString path, const Session::Type type)
{
    qDebug().nospace() << "Reading session (" << type << ") from " << path;

    KDesktopFile desktop(path); // NOTE: localises for us
    m_sessions << Session(type, path, desktop.readName(), desktop.readComment());
}
