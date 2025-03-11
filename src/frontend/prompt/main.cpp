#include <QGuiApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScreen>

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <kworkspace6/sessionmanagement.h>

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
    static void setTestModeEnabled(bool testModeEnabled);

private:
    void createWindowForScreen(QScreen *screen)
    {
        if (s_testMode && m_hasWindow) {
            return;
        }

        QQuickView *window = new QQuickView();
        window->QObject::setParent(this);
        window->setScreen(screen);

        if (s_testMode) {
            window->setColor(Qt::black);
            window->showFullScreen();
        } else {
            window->setColor(Qt::transparent);
            window->showFullScreen();
        }

        window->setSource(QUrl("qrc:/main.qml"));
        connect(qApp, &QGuiApplication::screenRemoved, this, [window, screen](QScreen *screenRemoved) {
            if (screenRemoved == screen) {
                delete window;
            }
        });

        m_hasWindow = true;
    }

    bool m_hasWindow = false;
    static bool s_testMode;
};

bool LoginGreeter::s_testMode = false;

void LoginGreeter::setTestModeEnabled(bool testModeEnabled)
{
    s_testMode = testModeEnabled;
}

int main(int argc, char *argv[])
{
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("test"), QStringLiteral("Run in test mode")));
    parser.addHelpOption();

    QGuiApplication app(argc, argv);
    parser.process(app);
    LoginGreeter::setTestModeEnabled(parser.isSet(QStringLiteral("test")));

    QQuickWindow::setDefaultAlphaBuffer(true);
    qmlRegisterSingletonInstance("org.greetd", 0, 1, "Authenticator", new GreetdLogin);

    // TODO: Singleton registered by registerSingletonInstance must only be accessed from one engine
    //       (is nullptr in other engines, hence only works on one window)
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionModel", new SDDM::SessionModel);
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "UserModel", new SDDM::UserModel(true));
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionManagement", new SessionManagement());

    LoginGreeter greeter;
    return app.exec();
}

#include "main.moc"
