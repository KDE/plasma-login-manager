/*
 * Qt Authentication Library
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "AuthPrompt.h"
#include "Auth.h"
#include "AuthMessages.h"

namespace PLASMALOGIN
{
class AuthPrompt::Private : public Prompt
{
public:
    Private(const Prompt *p)
    {
        // initializers are too mainstream i guess
        type = p->type;
        hidden = p->hidden;
        message = p->message;
        response = p->response;
    }
};

AuthPrompt::AuthPrompt(const Prompt *prompt, AuthRequest *parent)
    : QObject(parent)
    , d(new Private(prompt))
{
}

AuthPrompt::~AuthPrompt()
{
    delete d;
}

AuthPrompt::Type AuthPrompt::type() const
{
    return d->type;
}

QString AuthPrompt::message() const
{
    return d->message;
}

QByteArray AuthPrompt::response() const
{
    return d->response;
}

QByteArray AuthPrompt::responseFake()
{
    return QByteArray();
}

void AuthPrompt::setResponse(const QByteArray &r)
{
    if (r != d->response) {
        d->response = r;
        Q_EMIT responseChanged();
    }
}

bool AuthPrompt::hidden() const
{
    return d->hidden;
}
}

#include "moc_AuthPrompt.cpp"
