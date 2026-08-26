// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#pragma once

#include <memory>

#include <QObject>
#include <QProcess>
#include <QStringList>

namespace PLASMALOGIN
{

class Display;

class RunnableSession : public QObject
{
    Q_OBJECT
public:
    RunnableSession(std::unique_ptr<QProcess> process, QObject *parent = nullptr);
    void start();
    void stop();
    [[nodiscard]] bool isRunning() const;

Q_SIGNALS:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    std::unique_ptr<QProcess> m_process;
    std::optional<QString> m_unitName;
};

class SessionBuilder
{
public:
    [[nodiscard]] SessionBuilder &environment(const QProcessEnvironment &env);
    [[nodiscard]] SessionBuilder &name(const QString &unit);
    [[nodiscard]] SessionBuilder &description(const QString &description);
    [[nodiscard]] SessionBuilder &properties(const QStringList &properties);

    [[nodiscard]] std::unique_ptr<RunnableSession> build(const QString &command, const QStringList &arguments, Display *display);

private:
    std::optional<QProcessEnvironment> m_env;
    std::optional<QString> m_unitName;
    std::optional<QStringList> m_properties;
    std::optional<QString> m_description;
};

} // namespace PLASMALOGIN
