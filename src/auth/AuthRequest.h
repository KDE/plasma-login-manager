/*
 * Qt Authentication library
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <QtCore/QObject>

#include <QtQml/QQmlListProperty>

namespace PLASMALOGIN
{
class Auth;
class AuthPrompt;
class Request;
/**
 * \brief
 * AuthRequest is the main class for tracking requests from the underlying auth stack
 *
 * \section description
 * Typically, when logging in, you'll receive a list containing one or two fields:
 *
 *  * First one for the username (if you didn't provide it before);
 *    hidden = false, type = LOGIN_USER, message = whatever the stack provides
 *
 *  * Second one for the user's password
 *    hidden = true, type = LOGIN_PASSWORD, message = whatever the stack provides
 *
 * It's up to you to fill the \ref AuthPrompt::response property.
 * When all the fields are filled to your satisfaction, just trigger the \ref done
 * slot and the response will go back to the authenticator.
 *
 * \todo Decide if it's sane to use the info messages from PAM or to somehow parse them
 * and make the password changing message into a Request::Type of some kind
 */
class AuthRequest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<AuthPrompt> prompts READ promptsDecl NOTIFY promptsChanged)
    Q_PROPERTY(bool finishAutomatically READ finishAutomatically WRITE setFinishAutomatically NOTIFY finishAutomaticallyChanged)
public:
    /**
     * @return list of the contained prompts
     */
    QList<AuthPrompt *> prompts();
    /**
     * For QML apps
     * @return list of the contained prompts
     */
    QQmlListProperty<AuthPrompt> promptsDecl();

    static AuthRequest *empty();

    bool finishAutomatically();
    void setFinishAutomatically(bool value);
public Q_SLOTS:
    /**
     * Call this slot when all prompts has been filled to your satisfaction
     */
    void done();
Q_SIGNALS:
    /**
     * Emitted when \ref done was called
     */
    void finished();

    void finishAutomaticallyChanged();
    void promptsChanged();

private:
    AuthRequest(Auth *parent);
    void setRequest(const Request *request = nullptr);
    Request request() const;
    friend class Auth;
    class Private;
    Private *d{nullptr};
};
}

#endif // REQUEST_H
