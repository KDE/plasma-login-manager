/*
 * Base backend class to be inherited further
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef BACKEND_H
#define BACKEND_H

#include <QtCore/QObject>

namespace PLASMALOGIN
{
class HelperApp;
class Backend : public QObject
{
    Q_OBJECT
public:
    /**
     * Requests allocation of a new backend instance.
     * The method chooses the most suitable one for the current system.
     */
    static Backend *get(HelperApp *parent);

    void setAutologin(bool on = true);
    void setDisplayServer(bool on = true);
    void setGreeter(bool on = true);

public slots:
    virtual bool start(const QString &user = QString()) = 0;
    virtual bool authenticate() = 0;
    virtual bool openSession();
    virtual bool closeSession();

    virtual QString userName() = 0;

protected:
    Backend(HelperApp *parent);
    HelperApp *m_app;
    bool m_autologin{false};
    bool m_displayServer = false;
    bool m_greeter{false};
};
}

#endif // BACKEND_H
