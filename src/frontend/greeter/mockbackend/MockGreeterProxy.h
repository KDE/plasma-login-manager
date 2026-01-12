/*
 * SPDX-FileCopyrightText: David Edmundson
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once
#include <QObject>

#include "../backend/Messages.h"

// this class needs the same QML interface as GreeterProxy
class MockGreeterProxy : public QObject
{
    Q_OBJECT
public:
    MockGreeterProxy();
public Q_SLOTS:
    void login(const QString &user, const QString &password, const PLASMALOGIN::SessionType sessionType, const QString &sessionFileName) const;

Q_SIGNALS:
    void informationMessage(const QString &message);

    void socketDisconnected();
    void loginFailed();
    void loginSucceeded();

private:
    static constexpr QLatin1String s_mockPassword = QLatin1String("mypassword");
};
