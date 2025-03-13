#include <QObject>

// this class needs the same QML interface as GreeterProxy
class MockGreeterProxy : public QObject
{
    Q_OBJECT
public:
    MockGreeterProxy();
public Q_SLOTS:
    void login(const QString &user, const QString &password, const int sessionIndex);

Q_SIGNALS:
    void informationMessage(const QString &message);

    void socketDisconnected();
    void loginFailed();
    void loginSucceeded();
};
