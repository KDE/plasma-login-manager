/*
    SPDX-FileCopyrightText: 2013 Reza Fatahilah Shah <rshah0385@kireihana.com>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "plasmaloginsettings.h"
#include <KQuickManagedConfigModule>

class AdvancedConfig;
class PlasmaLoginData;

class PlasmaLoginKcm : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(PlasmaLoginSettings *plasmaLoginSettings READ plasmaLoginSettings CONSTANT)
public:
    explicit PlasmaLoginKcm(QObject *parent, const KPluginMetaData &metaData);
    ~PlasmaLoginKcm() override;

    Q_INVOKABLE static QString toLocalFile(const QUrl &url);
    Q_INVOKABLE void synchronizeSettings();
    Q_INVOKABLE void resetSyncronizedSettings();
    Q_INVOKABLE bool KDEWalletAvailable();
    Q_INVOKABLE void openKDEWallet();

    PlasmaLoginSettings *plasmaLoginSettings() const;
public Q_SLOTS:
    void save() override;
Q_SIGNALS:
    void errorOccured(const QString &untranslatedMessage);
    void syncAttempted();

private:
    PlasmaLoginData *m_data;
};

