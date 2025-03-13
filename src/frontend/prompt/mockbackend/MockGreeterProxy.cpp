#include "MockGreeterProxy.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

MockGreeterProxy::MockGreeterProxy()
{
    qDebug().noquote() << QStringLiteral("Mock backend in use, use password %1 for successful login on any user").arg(s_mockPassword);
}

void MockGreeterProxy::login(const QString &user, const QString &password, const int sessionIndex)
{
    bool const success = (!user.isEmpty() && password == s_mockPassword);

    qDebug().nospace() << "Login " << (success ? "success" : "failure") << " with user " << user << ", password " << password << ", session " << sessionIndex;

    if (success) {
        QTimer::singleShot(100, this, &MockGreeterProxy::loginSucceeded);
        QTimer::singleShot(800, []() { QCoreApplication::quit(); });
    } else {
        QTimer::singleShot(100, this, &MockGreeterProxy::loginFailed);
    }
}
