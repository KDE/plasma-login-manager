/*
 * Session process wrapper
 * SPDX-FileCopyrightText: 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 * SPDX-FileCopyrightText: 2014 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef PLASMALOGIN_AUTH_SESSION_H
#define PLASMALOGIN_AUTH_SESSION_H

#include <QtCore/QObject>
#include <QtCore/QProcess>

namespace PLASMALOGIN
{
class HelperApp;
class XOrgUserHelper;
class WaylandHelper;
class UserSession : public QProcess
{
    Q_OBJECT
public:
    explicit UserSession(HelperApp *parent);

    bool start();
    void stop();

    QString displayServerCommand() const;
    void setDisplayServerCommand(const QString &command);

    void setPath(const QString &path);
    QString path() const;

    /*!
     \brief Gets m_cachedProcessId
     \return  The cached process ID
    */
    qint64 cachedProcessId();

Q_SIGNALS:
    void finished(int exitCode);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
protected:
    void setupChildProcess() override;
#endif

private:
    void setup();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Don't call it directly, it will be invoked by the child process only
    void childModifier();
#endif

    QString m_path{};
    QString m_displayServerCmd;

    /*!
     Needed for getting the PID of a finished UserSession and calling HelperApp::utmpLogout
    */
    qint64 m_cachedProcessId = -1;
};
}

#endif // PLASMALOGIN_AUTH_SESSION_H
