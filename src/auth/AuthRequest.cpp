/*
 * Qt Authentication Library
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "AuthRequest.h"
#include "Auth.h"
#include "AuthMessages.h"

namespace PLASMALOGIN
{
class AuthRequest::Private : public QObject
{
    Q_OBJECT
public slots:
    void responseChanged();

public:
    Private(QObject *parent);
    QList<AuthPrompt *> prompts{};
    bool finishAutomatically{false};
    bool finished{true};
};

AuthRequest::Private::Private(QObject *parent)
    : QObject(parent)
{
}

void AuthRequest::Private::responseChanged()
{
    for (const AuthPrompt *qap : std::as_const(prompts)) {
        if (qap->response().isEmpty()) {
            return;
        }
    }
    if (finishAutomatically && prompts.length() > 0) {
        qobject_cast<AuthRequest *>(parent())->done();
    }
}

AuthRequest::AuthRequest(Auth *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

void AuthRequest::setRequest(const Request *request)
{
    QList<AuthPrompt *> promptsCopy(d->prompts);
    d->prompts.clear();
    if (request != nullptr) {
        for (const Prompt &p : std::as_const(request->prompts)) {
            AuthPrompt *qap = new AuthPrompt(&p, this);
            d->prompts << qap;
            if (finishAutomatically()) {
                connect(qap, &AuthPrompt::responseChanged, d, &AuthRequest::Private::responseChanged);
            }
        }
        d->finished = false;
    }
    Q_EMIT promptsChanged();
    if (request == nullptr) {
        qDeleteAll(promptsCopy);
    }
}

QList<AuthPrompt *> AuthRequest::prompts()
{
    return d->prompts;
}

QQmlListProperty<AuthPrompt> AuthRequest::promptsDecl()
{
    return QQmlListProperty<AuthPrompt>(this, &d->prompts);
}

void AuthRequest::done()
{
    if (!d->finished) {
        d->finished = true;
        Q_EMIT finished();
    }
}

bool AuthRequest::finishAutomatically()
{
    return d->finishAutomatically;
}

void AuthRequest::setFinishAutomatically(bool value)
{
    if (value != d->finishAutomatically) {
        d->finishAutomatically = value;
        Q_EMIT finishAutomaticallyChanged();
    }
}

Request AuthRequest::request() const
{
    Request r;
    for (const AuthPrompt *qap : std::as_const(d->prompts)) {
        Prompt p;
        p.hidden = qap->hidden();
        p.message = qap->message();
        p.response = qap->response();
        p.type = qap->type();
        r.prompts << p;
    }
    return r;
}
}

#include "AuthRequest.moc"

#include "moc_AuthRequest.cpp"
