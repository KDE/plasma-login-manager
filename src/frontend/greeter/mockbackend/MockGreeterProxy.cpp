/*
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "MockGreeterProxy.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

MockGreeterProxy::MockGreeterProxy()
{
    qDebug().noquote() << QStringLiteral("Mock backend in use, use password %1 for successful login on any user").arg(s_mockPassword);
}

void MockGreeterProxy::login(const QString &user, const QString &password, const PLASMALOGIN::SessionType sessionType, const QString &sessionFileName) const
{
    bool const success = (!user.isEmpty() && password == s_mockPassword);

    QString sessionTypeName;
    switch (sessionType) {
    case PLASMALOGIN::SessionType::X11:
        sessionTypeName = QStringLiteral("X11");
        break;
    case PLASMALOGIN::SessionType::Wayland:
        sessionTypeName = QStringLiteral("Wayland");
        break;
    }

    qDebug().nospace() << "Login " << (success ? "success" : "failure") << " with user " << user << ", password " << password << ", session " << sessionTypeName
                       << " " << sessionFileName;

    if (success) {
        QTimer::singleShot(100, this, &MockGreeterProxy::loginSucceeded);
        QTimer::singleShot(800, []() {
            QCoreApplication::quit();
        });
    } else {
        QTimer::singleShot(100, this, &MockGreeterProxy::loginFailed);
    }
}

void MockGreeterProxy::selectUser(const QString &user, const PLASMALOGIN::SessionType sessionType, const QString &sessionFileName, bool active) const
{
    Q_UNUSED(user)
    Q_UNUSED(sessionType)
    Q_UNUSED(sessionFileName)
    Q_UNUSED(active)
}

#include "moc_MockGreeterProxy.cpp"
