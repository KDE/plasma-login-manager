/*
    SPDX-FileCopyrightText: 2019 Filip Fila <filipfila.kde@gmail.com>
    SPDX-FileCopyrightText: 2013 Reza Fatahilah Shah <rshah0385@kireihana.com>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KAuth/ActionReply>
#include <KAuth/HelperSupport>
#include <QObject>

using namespace KAuth;

class PlasmaLoginAuthHelper : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    /*
     * Copy the user's Plasma configuration (e.g. Displays, fonts, colors) to the plasmalogin user
     */
    ActionReply sync(const QVariantMap &args);

    /*
     * Remove any Plasma configuration copied to the plasmalogin user
     */
    ActionReply reset(const QVariantMap &args);

    /*
     * Update the PLASMALOGIN_CONFIG_FILE with the user's specified settings
     */
    ActionReply save(const QVariantMap &args);

};
