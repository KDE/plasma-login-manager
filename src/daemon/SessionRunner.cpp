// Minimal implementation for SessionRunner interface

#include "SessionRunner.h"
#include <qdbusmetatype.h>

#include "managerinterface.h"

namespace PLASMALOGIN
{
SessionRunner::SessionRunner(QObject *parent)
    : QObject(parent)
{
}

SessionRunner::~SessionRunner() = default;

void SessionRunner::setUser(const QString &user)
{
    m_user = user;
}

void SessionRunner::setGreeter(bool greeter)
{
    m_isGreeter = greeter;
}

void SessionRunner::setExecutable(const QString &session)
{
    m_sessionPath = session;
}

void SessionRunner::insertEnvironment(const QProcessEnvironment &env)
{
    m_environment = env;
}

void SessionRunner::start() {
    QStringList args = m_sessionPath.split(' ');
    QString executable = args.first();

    qDBusRegisterMetaType<QVariantMultiItem>();
    qDBusRegisterMetaType<QVariantMultiMap>();
    qDBusRegisterMetaType<TransientAux>();
    qDBusRegisterMetaType<TransientAuxList>();
    qDBusRegisterMetaType<ExecCommand>();
    qDBusRegisterMetaType<ExecCommandList>();

    const auto systemdService = QStringLiteral("org.freedesktop.systemd1");
    const auto systemdPath = QStringLiteral("/org/freedesktop/systemd1");

    org::freedesktop::systemd1::Manager manager(systemdService, systemdPath, QDBusConnection::systemBus(), nullptr);

    QVariantMultiMap properties = {
        // Unit properties
        {QStringLiteral("Type"), QStringLiteral("simple")},
        {QStringLiteral("Description"), QStringLiteral("Plasma Login Session Launcher")},
        // {QStringLiteral("AddRef"), true},

        {QStringLiteral("Conflicts"), QStringList({QStringLiteral("getty@tty1.service")})},
        // {QStringLiteral("After"), QStringList({QStringLiteral("getty@tty1.service")})},

        // User context
        {QStringLiteral("User"), m_user},
        {QStringLiteral("PAMName"), QStringLiteral("login")}, // should be the pam file for autologin

        // Environment
        {QStringLiteral("Environment"), m_environment.toStringList()},

        // Working directory
        // {QStringLiteral("WorkingDirectory"), QStringLiteral("/home/david")},

        // TTY settings
        {QStringLiteral("TTYPath"), QStringLiteral("/dev/tty1")},
        {QStringLiteral("TTYReset"), true},
        {QStringLiteral("TTYVHangup"), true},
        {QStringLiteral("TTYVTDisallocate"), true},
        {QStringLiteral("StandardInput"), QStringLiteral("tty-fail")},
        {QStringLiteral("StandardOutput"), QStringLiteral("journal")},
        {QStringLiteral("StandardError"), QStringLiteral("journal")},

        // Utmp tracking
        {QStringLiteral("UtmpIdentifier"), QStringLiteral("tty1")},
        {QStringLiteral("UtmpMode"), QStringLiteral("user")},

        {QStringLiteral("Restart"), QStringLiteral("no")},

        // ExecStart: [path, args, ignore-failure]
        {QStringLiteral("ExecStart"), QVariant::fromValue(ExecCommandList{{executable, args, false}})},
    };
    qDebug() << executable;


    static int i = 0;
    i++;
    const QString unit = QStringLiteral("plasma-login-test%1.service").arg(i); //TODO, user name + VT
    auto reply = manager.StartTransientUnit(unit, QStringLiteral("replace"), properties, {});

    reply.waitForFinished();
    qDebug() << reply.error().message();
}

}

#include "moc_SessionRunner.cpp"
