/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDBusObjectPath>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include <optional>

class ExistingSessionTracker : public QObject
{
    Q_OBJECT

public:
    explicit ExistingSessionTracker(QObject *parent = nullptr);
    ~ExistingSessionTracker() override = default;

    QString currentSeat();
    QStringList graphicalSessionSeatsForUser(uint uid);

Q_SIGNALS:
    void graphicalSessionsChanged(uint uid);

private Q_SLOTS:
    void sessionNew(const QString &, const QDBusObjectPath &path);
    void sessionRemoved(const QString &, const QDBusObjectPath &path);

private:
    struct Session {
        uint uid = 0;
        QString seat;
    };

    void ensureLoaded();
    void loadSessions();

    std::optional<Session> readSession(const QDBusObjectPath &path);
    QString readCurrentSeat() const;

    bool m_loaded = false;
    QString m_currentSeat;
    QHash<QString, Session> m_sessions;
};
