#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QObject>
#include <QScreen>

#include <kworkspace6/sessionmanagement.h>
#include <KWindowSystem>
#include <LayerShellQt/Window>
#include <PlasmaQuick/QuickViewSharedEngine>

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

        auto *window = new PlasmaQuick::QuickViewSharedEngine();
        window->QObject::setParent(this);
        window->setScreen(screen);
        window->setColor(s_testMode ? Qt::black : Qt::transparent);

        window->setGeometry(screen->geometry());
        connect(screen, &QScreen::geometryChanged, this, [window]() {
            window->setGeometry(window->screen()->geometry());
        });

        if (KWindowSystem::isPlatformWayland()) {
            if (auto layerShellWindow = LayerShellQt::Window::get(window)) {
                layerShellWindow->setScope(QStringLiteral("plasma-login-wallpaper"));
                layerShellWindow->setLayer(LayerShellQt::Window::LayerTop);
                layerShellWindow->setExclusiveZone(-1);
                layerShellWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
            }
        }

        window->setResizeMode(PlasmaQuick::QuickViewSharedEngine::SizeRootObjectToView);

        if (KWindowSystem::isPlatformX11()) {
            // X11 specific hint only on X11
            window->setFlags(Qt::BypassWindowManagerHint);
        } else if (!KWindowSystem::isPlatformWayland()) {
            // on other platforms go fullscreen
            // on Wayland we cannot go fullscreen due to QTBUG 54883
            window->setWindowState(Qt::WindowFullScreen);
        }

        window->setSource(QUrl("qrc:/main.qml"));
        connect(qApp, &QGuiApplication::screenRemoved, this, [window](QScreen *screenRemoved) {
            if (screenRemoved == window->screen()) {
                delete window;
            }
        });

        window->show();

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
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionModel", new SDDM::SessionModel);
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "UserModel", new SDDM::UserModel(true));
    qmlRegisterSingletonInstance("org.kde.plasma.login", 0, 1, "SessionManagement", new SessionManagement());

    LoginGreeter greeter;
    return app.exec();
}

#include "main.moc"
