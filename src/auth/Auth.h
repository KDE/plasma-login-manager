/*
 * Qt Authentication library
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef PLASMALOGIN_AUTH_H
#define PLASMALOGIN_AUTH_H

#include "AuthPrompt.h"
#include "AuthRequest.h"

#include <QtCore/QObject>
#include <QtCore/QProcessEnvironment>

namespace PLASMALOGIN
{
/**
 * \brief
 * Main class triggering the authentication and handling all communication
 *
 * \section description
 * There are three basic kinds of authentication:
 *
 *  * Checking only the validity of the user's secrets - The default values
 *
 *  * Logging the user in after authenticating him - You'll have to set the
 *      \ref session property to do that.
 *
 *  * Logging the user in without authenticating - You'll have to set the
 *      \ref session and \ref autologin properties to do that.
 *
 * Usage:
 *
 * Just construct, connect the signals (especially \ref requestChanged)
 * and fire up \ref start
 */
class Auth : public QObject
{
    Q_OBJECT
public:
    explicit Auth(QObject *parent = nullptr);
    ~Auth();

    enum Info {
        INFO_NONE = 0,
        INFO_UNKNOWN,
        INFO_PASS_CHANGE_REQUIRED,
        _INFO_LAST
    };
    Q_ENUM(Info)

    enum Error {
        ERROR_NONE = 0,
        ERROR_UNKNOWN,
        ERROR_AUTHENTICATION,
        ERROR_INTERNAL,
        _ERROR_LAST
    };
    Q_ENUM(Error)

    enum HelperExitStatus {
        HELPER_SUCCESS = 0,
        HELPER_AUTH_ERROR,
        HELPER_SESSION_ERROR,
        HELPER_OTHER_ERROR,
        HELPER_DISPLAYSERVER_ERROR,
        HELPER_TTY_ERROR,
    };
    Q_ENUM(HelperExitStatus)

    static void registerTypes();

    bool autologin() const;
    bool isGreeter() const;
    const QString &user() const;
    const QString &session() const;
    AuthRequest *request();
    /**
     * True if an authentication or session is in progress
     */
    bool isActive() const;

    /**
     * Sets the user which will then authenticate
     * @param user username
     */
    void setUser(const QString &user);

public Q_SLOTS:
    /**
     * Sets up the environment and starts the authentication
     */
    void start();

    /**
     * Indicates that we do not need the process anymore.
     */
    void stop();

Q_SIGNALS:

    void requestChanged();
    /**
     * Emitted when authentication phase finishes
     *
     * @note If you want to set some environment variables for the session right before the
     * session is started, connect to this signal using a blocking connection and insert anything
     * you need in the slot.
     * @param user username
     * @param success true if succeeded
     */
    void authentication(QString user, bool success);

    /**
     * Emitted on error
     *
     * @param message message to be displayed to the user
     */
    void error(QString message, Auth::Error type);

    /**
     * Information from the underlying stack is to be presented to the user
     *
     * @param message message to be displayed to the user
     */
    void info(QString message, Auth::Info type);

private:
    class Private;
    class SocketServer;
    friend Private;
    friend SocketServer;
    Private *d{nullptr};
};
}

#endif // PLASMALOGIN_AUTH_H
