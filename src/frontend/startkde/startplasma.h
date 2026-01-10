/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

// #include "config-startplasma.h"
// #include "kcheckrunning/kcheckrunning.h"
// #include <ksplashinterface.h>
#include <optional>

#include <QProcessEnvironment>
#include <QString>
#include <QTextStream>

extern QTextStream out;

void sigtermHandler(int signalNumber);
QStringList allServices(const QLatin1String &prefix);
int runSync(const QString &program, const QStringList &args, const QStringList &env = {});

void createConfigDirectory();
void runStartupConfig();
void setupCursor(bool wayland);
std::optional<QProcessEnvironment> getSystemdEnvironment();
void importSystemdEnvrionment();
void runEnvironmentScripts();
void setupPlasmaEnvironment();
void cleanupPlasmaEnvironment(const std::optional<QProcessEnvironment> &oldSystemdEnvironment);
bool syncDBusEnvironment();
void setupFontDpi();
QProcess *setupKSplash();

bool startPlasmaSession(bool wayland);

void stopSystemdSession();
void waitForKonqi();

void gentleTermination(QProcess *process);

struct KillBeforeDeleter {
    void operator()(QProcess *pointer)
    {
        if (pointer) {
            gentleTermination(pointer);
        }
        delete pointer;
    }
};
