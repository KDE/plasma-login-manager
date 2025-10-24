// Minimal implementation for SessionRunner interface

#include "SessionRunner.h"
#include <qdbusmetatype.h>

#include "managerinterface.h"
#include <QDBusMessage>
#include <QDBusPendingReply>

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
    m_executable = session;
}

void SessionRunner::insertEnvironment(const QProcessEnvironment &env)
{
    m_environment = env;
}

void SessionRunner::setTty(int tty)
{
    m_tty = tty;
}

bool SessionRunner::start()
{
    QStringList args = m_executable.split(' ');
    QString executable = args.first();

    if (executable.isEmpty()) {
        qCritical() << "SessionRunner: refusing to start an empty command";
        return false;
    }

    if (m_tty <= 0) {
        qCritical() << "SessionRunner: refusing to start" << executable << "with invalid tty" << m_tty;
        return false;
    }

    if (!m_unit.isEmpty()) {
        qWarning() << "SessionRunner: unit" << m_unit << "already running, stopping it before restart";
        const bool stopped = stop();
        Q_UNUSED(stopped);
    }

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

        {QStringLiteral("Conflicts"), QStringList({QStringLiteral("getty@tty%1.service").arg(m_tty)})},

        // User context
        {QStringLiteral("User"), m_user},
        {QStringLiteral("PAMName"), QStringLiteral("plasmalogin-greeter")}, // this needs changing for autologin and regular sessions

        // Environment
        {QStringLiteral("Environment"), m_environment.toStringList()},

        // TTY settings
        {QStringLiteral("TTYPath"), QStringLiteral("/dev/tty%1").arg(m_tty)},
        {QStringLiteral("TTYReset"), true},
        {QStringLiteral("TTYVHangup"), true},
        {QStringLiteral("TTYVTDisallocate"), true},
        {QStringLiteral("StandardInput"), QStringLiteral("tty-fail")},
        {QStringLiteral("StandardOutput"), QStringLiteral("journal")},
        {QStringLiteral("StandardError"), QStringLiteral("journal")},

        // Utmp tracking
        {QStringLiteral("UtmpIdentifier"), QStringLiteral("tty%1").arg(m_tty)},
        {QStringLiteral("UtmpMode"), QStringLiteral("user")},

        {QStringLiteral("Restart"), QStringLiteral("no")},

        // ExecStart: [path, args, ignore-failure]
        {QStringLiteral("ExecStart"), QVariant::fromValue(ExecCommandList{{executable, args, false}})},
    };

    static int i = 0;
    i++;
    const QString unit = QStringLiteral("plasma-login-%1-%2.service").arg(m_isGreeter ? QStringLiteral("greeter") : QStringLiteral("session")).arg(i);
    qDebug() << "SessionRunner starting transient unit" << unit << "for user" << m_user << "on tty" << m_tty << "command"
             << args;
    auto reply = manager.StartTransientUnit(unit, QStringLiteral("replace"), properties, {});

    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "SessionRunner failed to start transient unit" << unit << reply.error().name() << reply.error().message();
        return false;
    }

    m_unit = unit;
    qDebug() << "SessionRunner started transient unit" << unit;
    return true;
}

bool SessionRunner::stop()
{
    if (m_unit.isEmpty()) {
        return false;
    }

    qDebug() << "SessionRunner stopping transient unit" << m_unit;
    QDBusMessage stopMessage = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                              QStringLiteral("/org/freedesktop/systemd1"),
                                                              QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                              QStringLiteral("StopUnit"));
    stopMessage << m_unit << QStringLiteral("replace");
    QDBusPendingReply<QDBusObjectPath> reply = QDBusConnection::systemBus().asyncCall(stopMessage);
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "SessionRunner failed to stop transient unit" << m_unit << reply.error().name() << reply.error().message();
        return false;
    }

    qDebug() << "SessionRunner stopped transient unit" << m_unit;
    m_unit.clear();
    return true;
}

bool SessionRunner::isRunning() const
{
    return !m_unit.isEmpty();
}

}

#include "moc_SessionRunner.cpp"
