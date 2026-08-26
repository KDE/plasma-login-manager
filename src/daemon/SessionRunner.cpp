// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include "SessionRunner.h"

#include <QDebug>
#include <QJsonDocument>

#include "DaemonApp.h"
#include "Display.h"
#include "DisplayManager.h"
#include "MainConfigLoader.h"
#include "Seat.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;
using namespace PLASMALOGIN;

PLASMALOGIN::RunnableSession::RunnableSession(std::unique_ptr<QProcess> process, QObject *parent)
    : QObject(parent)
    , m_process(std::move(process))
{
    connect(m_process.get(), &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        switch (exitStatus) {
        case QProcess::NormalExit:
            break;
        case QProcess::CrashExit:
            Q_EMIT finished(exitCode, exitStatus);
            return;
        }

        if (exitCode != 0) {
            qWarning() << "RunnableSession exited with non-zero exit code" << exitCode;
            Q_EMIT finished(exitCode, exitStatus);
            return;
        }

        m_unitName = [&]() -> std::optional<QString> {
            auto doc = QJsonDocument::fromJson(m_process->readAllStandardOutput());
            if (doc.isObject() && doc.object().contains("unit")) {
                return doc.object().value("unit").toString();
            }
            return std::nullopt;
        }();
    });
}

void PLASMALOGIN::RunnableSession::start()
{
    m_process->start();
}

void PLASMALOGIN::RunnableSession::stop()
{
    if (!m_unitName) {
        qWarning() << "RunnableSession::stop called but no unit name is set, cannot stop session";
        return;
    }

    QProcess term;
    term.setProgram(u"systemctl"_s);
    term.setArguments({u"kill"_s, u"--wait"_s, u"--kill-whom=all"_s, u"--signal=SIGTERM"_s, *m_unitName});
    term.start();
    if (!term.waitForFinished((250ms).count())) {
        qWarning() << "Greeter did not stop in time, sending SIGKILL";
        QProcess kill;
        kill.setProgram(u"systemctl"_s);
        kill.setArguments({u"kill"_s, u"--wait"_s, u"--kill-whom=all"_s, u"--signal=SIGKILL"_s, *m_unitName});
        kill.start();
        if (!kill.waitForFinished((100ms).count())) {
            qWarning() << "Greeter did not stop in time, killing it";
        }
    }
}

bool PLASMALOGIN::RunnableSession::isRunning() const
{
    if (m_process->state() == QProcess::Running) {
        return true;
    }

    if (!m_unitName) {
        return false;
    }
    return QProcess::execute(u"systemctl"_s, {u"is-active"_s, *m_unitName}) == 0;
}

SessionBuilder &SessionBuilder::environment(const QProcessEnvironment &env)
{
    m_env = env;
    return *this;
}

PLASMALOGIN::SessionBuilder &PLASMALOGIN::SessionBuilder::name(const QString &unit)
{
    m_unitName = unit;
    return *this;
}

[[nodiscard]] PLASMALOGIN::SessionBuilder &PLASMALOGIN::SessionBuilder::description(const QString &description)
{
    m_description = description;
    return *this;
}

[[nodiscard]] PLASMALOGIN::SessionBuilder &PLASMALOGIN::SessionBuilder::properties(const QStringList &properties)
{
    m_properties = properties;
    return *this;
}

[[nodiscard]] std::unique_ptr<RunnableSession> PLASMALOGIN::SessionBuilder::build(const QString &command, const QStringList &commandArguments, Display *display)
{
    auto process = std::make_unique<QProcess>();
    process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process->setProgram(u"systemd-run"_s);
    process->setArguments([&] {
        auto arguments = QStringList{
            u"--service-type=simple"_s,
            u"--slice-inherit"_s,
            u"--collect"_s, // Remove the unit after we are done
            u"--property=Restart=no"_s, // Process gets managed by the Greeter class, not systemd

            // Pre-fill always relevant variables
            u"--setenv=PATH="_s + PlasmaLogin::config()->defaultPath(),
            u"--setenv=XDG_SEAT="_s + display->seat()->name(),
            u"--setenv=XDG_SEAT_PATH="_s + daemonApp->displayManager()->seatPath(display->seat()->name()),
            u"--setenv=XDG_SESSION_PATH="_s + daemonApp->displayManager()->sessionPath(QStringLiteral("Session%1").arg(daemonApp->newSessionId())),

            u"--json=short"_s,
        };

        if (m_unitName) {
            arguments.append(u"--unit="_s + *m_unitName);
        }

        if (m_description) {
            arguments.append(u"--description="_s + *m_description);
        }

        if (m_env) {
            for (const auto &var : m_env->toStringList()) {
                arguments.append(u"--setenv="_s + var);
            }
        }

        if (m_properties) {
            for (const auto &property : *m_properties) {
                arguments.append(u"--property="_s + property);
            }
        }

        arguments.append(command);
        arguments.append(commandArguments);
        return arguments;
    }());

    return std::make_unique<RunnableSession>(std::move(process));
}
