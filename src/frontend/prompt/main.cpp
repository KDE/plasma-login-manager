#include <QGuiApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScreen>

#include "greetd/GreetdManager.hpp"
#include "SessionModel.h"
#include "UserModel.h"

class LoginGreeter : public QObject
{
    Q_OBJECT
public:
    explicit LoginGreeter(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
            createWindowForScreen(screen);
        });
        for (QScreen *screen : qApp->screens()) {
            createWindowForScreen(screen);
        }
    }

private:
    void createWindowForScreen(QScreen *screen)
    {
        QQuickView *window = new QQuickView();
        window->QObject::setParent(this);
        window->setScreen(screen);
        window->showFullScreen();
        window->setSource(QUrl("qrc:/main.qml"));
        connect(qApp, &QGuiApplication::screenRemoved, this, [this, window, screen](QScreen *screenRemoved) {
            if (screenRemoved == screen) {
                delete window;
            }
        });
    }

private Q_SLOTS:
    void onScreenAdded(QScreen *screen)
    {
        createWindowForScreen(screen);
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonInstance("org.greetd", 0, 1, "Authenticator", new GreetdLogin);

    // TODO: Singleton registered by registerSingletonInstance must only be accessed from one engine
    //       (is nullptr in other engines, hence only works on one window)
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionModel", new SDDM::SessionModel);
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "UserModel", new SDDM::UserModel(true));

    LoginGreeter greeter;
    return app.exec();
}

#include "main.moc"
