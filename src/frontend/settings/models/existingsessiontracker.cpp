/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "existingsessiontracker.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QMetaType>
#include <QSignalBlocker>
#include <QVariantMap>

namespace
{
using namespace Qt::StringLiterals;

constexpr QLatin1StringView Login1Service = "org.freedesktop.login1"_L1;
constexpr QLatin1StringView Login1ManagerPath = "/org/freedesktop/login1"_L1;
constexpr QLatin1StringView Login1ManagerInterface = "org.freedesktop.login1.Manager"_L1;
constexpr QLatin1StringView Login1SessionInterface = "org.freedesktop.login1.Session"_L1;
constexpr QLatin1StringView DBusPropertiesInterface = "org.freedesktop.DBus.Properties"_L1;

QVariant unwrapDBusVariant(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }

    return value;
}

std::optional<uint> uidFromProperty(const QVariant &value)
{
    const QVariant unwrappedValue = unwrapDBusVariant(value);
    if (unwrappedValue.metaType() != QMetaType::fromType<QDBusArgument>()) {
        return {};
    }

    const QDBusArgument argument = unwrappedValue.value<QDBusArgument>();
    if (argument.currentType() != QDBusArgument::StructureType) {
        return {};
    }

    uint uid = 0;
    QDBusObjectPath path;
    argument.beginStructure();
    argument >> uid >> path;
    argument.endStructure();

    return uid;
}

QString seatFromProperty(const QVariant &value)
{
    const QVariant unwrappedValue = unwrapDBusVariant(value);
    if (unwrappedValue.metaType() != QMetaType::fromType<QDBusArgument>()) {
        return {};
    }

    const QDBusArgument argument = unwrappedValue.value<QDBusArgument>();
    if (argument.currentType() != QDBusArgument::StructureType) {
        return {};
    }

    QString seat;
    QDBusObjectPath path;
    argument.beginStructure();
    argument >> seat >> path;
    argument.endStructure();

    return seat;
}
}

ExistingSessionTracker::ExistingSessionTracker(QObject *parent)
    : QObject(parent)
{
}

QString ExistingSessionTracker::currentSeat()
{
    ensureLoaded();
    return m_currentSeat;
}

QStringList ExistingSessionTracker::graphicalSessionSeatsForUser(uint uid)
{
    ensureLoaded();

    QStringList seats;
    for (auto it = m_sessions.cbegin(); it != m_sessions.cend(); ++it) {
        const Session &session = it.value();
        if (session.uid != uid) {
            continue;
        }

        seats.append(session.seat);
    }

    return seats;
}

void ExistingSessionTracker::ensureLoaded()
{
    if (m_loaded) {
        return;
    }

    m_loaded = true;

    if (!QDBusConnection::systemBus().isConnected()) {
        qWarning() << "Failed to connect to the system bus for logind session tracking";
        return;
    }

    m_currentSeat = readCurrentSeat();
    if (m_currentSeat.isEmpty()) {
        qWarning() << "Failed to determine current logind seat";
    }

    const bool connectedSessionNew = QDBusConnection::systemBus().connect(Login1Service,
                                                                          Login1ManagerPath,
                                                                          Login1ManagerInterface,
                                                                          QStringLiteral("SessionNew"),
                                                                          this,
                                                                          SLOT(sessionNew(QString, QDBusObjectPath)));
    const bool connectedSessionRemoved = QDBusConnection::systemBus().connect(Login1Service,
                                                                              Login1ManagerPath,
                                                                              Login1ManagerInterface,
                                                                              QStringLiteral("SessionRemoved"),
                                                                              this,
                                                                              SLOT(sessionRemoved(QString, QDBusObjectPath)));

    if (!connectedSessionNew || !connectedSessionRemoved) {
        qWarning() << "Failed to subscribe to logind session changes";
    }

    loadSessions();
}

void ExistingSessionTracker::loadSessions()
{
    const QSignalBlocker blocker(this);

    QDBusInterface manager(Login1Service, Login1ManagerPath, Login1ManagerInterface, QDBusConnection::systemBus());
    QDBusMessage reply = manager.call(QStringLiteral("ListSessions"));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        qWarning() << "Failed to list logind sessions:" << reply.errorMessage();
        return;
    }

    const QVariant sessionsVariant = reply.arguments().constFirst();
    if (sessionsVariant.metaType() != QMetaType::fromType<QDBusArgument>()) {
        qWarning() << "Failed to parse logind session list";
        return;
    }

    const QDBusArgument sessionsArgument = sessionsVariant.value<QDBusArgument>();
    sessionsArgument.beginArray();
    while (!sessionsArgument.atEnd()) {
        QString sessionId;
        uint uid = 0;
        QString userName;
        QString seat;
        QDBusObjectPath path;

        sessionsArgument.beginStructure();
        sessionsArgument >> sessionId >> uid >> userName >> seat >> path;
        sessionsArgument.endStructure();

        sessionNew(sessionId, path);
    }
    sessionsArgument.endArray();
}

std::optional<ExistingSessionTracker::Session> ExistingSessionTracker::readSession(const QDBusObjectPath &path)
{
    if (path.path().isEmpty()) {
        return {};
    }

    QDBusInterface properties(Login1Service, path.path(), DBusPropertiesInterface, QDBusConnection::systemBus());
    QDBusReply<QVariantMap> reply = properties.call(QStringLiteral("GetAll"), Login1SessionInterface);
    if (!reply.isValid()) {
        qDebug() << "Failed to read logind session properties:" << path.path() << reply.error().message();
        return {};
    }

    const QVariantMap propertiesMap = reply.value();

    const std::optional<uint> uid = uidFromProperty(propertiesMap.value(QStringLiteral("User")));
    if (!uid) {
        return {};
    }

    const QString type = propertiesMap.value(QStringLiteral("Type")).toString();
    const QString sessionClass = propertiesMap.value(QStringLiteral("Class")).toString();
    if (sessionClass != QLatin1String("user") || (type != QLatin1String("wayland") && type != QLatin1String("x11"))) {
        return {};
    }

    return Session{*uid, seatFromProperty(propertiesMap.value(QStringLiteral("Seat")))};
}

QString ExistingSessionTracker::readCurrentSeat() const
{
    QDBusInterface properties(Login1Service, QStringLiteral("/org/freedesktop/login1/session/auto"), DBusPropertiesInterface, QDBusConnection::systemBus());
    QDBusReply<QDBusVariant> reply = properties.call(QStringLiteral("Get"), Login1SessionInterface, QStringLiteral("Seat"));
    if (!reply.isValid()) {
        qDebug() << "Failed to read current logind session seat:" << reply.error().message();
        return {};
    }

    return seatFromProperty(reply.value().variant());
}

void ExistingSessionTracker::sessionNew(const QString &, const QDBusObjectPath &path)
{
    const std::optional<Session> session = readSession(path);
    if (!session) {
        return;
    }

    m_sessions.insert(path.path(), *session);
    Q_EMIT graphicalSessionsChanged(session->uid);
}

void ExistingSessionTracker::sessionRemoved(const QString &, const QDBusObjectPath &path)
{
    const auto sessionIt = m_sessions.find(path.path());
    if (sessionIt == m_sessions.end()) {
        return;
    }

    const uint uid = sessionIt->uid;
    m_sessions.erase(sessionIt);
    Q_EMIT graphicalSessionsChanged(uid);
}

#include "moc_existingsessiontracker.cpp"
