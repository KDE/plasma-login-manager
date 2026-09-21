/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>
    SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class BlurAdaptor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString activeScreen READ activeScreen NOTIFY activeScreenChanged)

public:
    explicit BlurAdaptor(QObject *parent = nullptr);

    QString activeScreen() const
    {
        return m_activeScreen;
    }

Q_SIGNALS:
    void activeScreenChanged();

public Q_SLOTS:
    Q_SCRIPTABLE void blurScreen(const QString &screenName);

private:
    QString m_activeScreen;
};
