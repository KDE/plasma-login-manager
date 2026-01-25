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
    explicit Auth(const QString &user = QString(), const QString &session = QString(), bool autologin = false, QObject *parent = 0, bool verbose = false);
    explicit Auth(QObject *parent);
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
    bool verbose() const;
    const QString &user() const;
    const QString &session() const;
    AuthRequest *request();
    /**
     * True if an authentication or session is in progress
     */
    bool isActive() const;

    /**
     * If starting a session, you will probably want to provide some basic env variables for the session.
     * This only inserts the variables - if the current key already had a value, it will be overwritten.
     * User-specific data such as $HOME is generated automatically.
     * @param env the environment
     */
    void insertEnvironment(const QProcessEnvironment &env);

    /**
     * Works the same as \ref insertEnvironment but only for one key-value pair
     * @param key key
     * @param value value
     */
    void insertEnvironment(const QString &key, const QString &value);

    /**
     * Set mode to autologin.
     * Ignored if session is not started
     * @param on true if should autologin
     */
    void setAutologin(bool on = true);

    /**
     * Set mode to greeter
     * This will bypass authentication checks
     */
    void setGreeter(bool on = true);

    /**
     * Forwards the output of the underlying authenticator to the current process
     * @param on true if should forward the output
     */
    void setVerbose(bool on = true);

    /**
     * Sets the user which will then authenticate
     * @param user username
     */
    void setUser(const QString &user);

    /**
     * Set the session to be started after authenticating.
     * @param path Path of the session executable to be started
     */
    void setSession(const QString &path);

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
    void autologinChanged();
    void greeterChanged();
    void verboseChanged();
    void userChanged();
    void displayServerCommandChanged();
    void sessionChanged();
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
     * Emitted when session starting phase finishes
     *
     * @param success true if succeeded
     */
    void sessionStarted(bool success);

    /**
     * Emitted when the display server is ready.
     *
     * @param displayName display name
     */
    void displayServerReady(const QString &displayName);

    /**
     * Emitted when the helper quits, either after authentication or when the session ends.
     * Or, when something goes wrong.
     *
     * @param success true if every underlying task went fine
     */
    void finished(Auth::HelperExitStatus status);

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
