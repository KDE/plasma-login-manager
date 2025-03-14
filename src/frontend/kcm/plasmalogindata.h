/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KCModuleData>

class PlasmaLoginSettings;

class PlasmaLoginData : public KCModuleData
{
    Q_OBJECT
public:
    PlasmaLoginData(QObject *parent);
    PlasmaLoginSettings *plasmaLoginSettings() const;

private:
    PlasmaLoginSettings *m_settings;
};
