#include "MockGreeterProxy.h"

#include <QTextStream>
#include <QDebug>
#include <QTimer>

QTextStream& qStdOut()
{
    static QTextStream ts( stdout );
    return ts;
}

MockGreeterProxy::MockGreeterProxy()
{

}

void MockGreeterProxy::login(const QString &user, const QString &password, const int sessionIndex)
{
    Q_UNUSED(sessionIndex);
    qStdOut() << "Login attempt. User: " << user << " Password: " << password;

    if(password == "mypassword" && !user.isEmpty()) {
        QTimer::singleShot(100, this, &MockGreeterProxy::loginSucceeded);
    } else {
        qDebug() << "Use password 'mypassword' for successful login";
        QTimer::singleShot(100, this, &MockGreeterProxy::loginFailed);
    }
}
