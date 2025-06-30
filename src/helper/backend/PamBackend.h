/*
 * PAM authentication backend
 * SPDX-FileCopyrightText: 2013 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#if !defined(PAMBACKEND_H)
#define PAMBACKEND_H

#include "../Backend.h"
#include "AuthMessages.h"
#include "Constants.h"

#include <QtCore/QObject>

#include <security/pam_appl.h>

namespace PLASMALOGIN
{
class PamHandle;
class PamBackend;
class PamData
{
public:
    PamData();

    bool insertPrompt(const struct pam_message *msg, bool predict = true);
    Auth::Info handleInfo(const struct pam_message *msg, bool predict);

    const Request &getRequest() const;
    void completeRequest(const Request &request);

    QByteArray getResponse(const struct pam_message *msg);

private:
    AuthPrompt::Type detectPrompt(const struct pam_message *msg) const;

    const Prompt &findPrompt(const struct pam_message *msg) const;
    Prompt &findPrompt(const struct pam_message *msg);

    bool m_sent{false};
    Request m_currentRequest{};
};

class PamBackend : public Backend
{
    Q_OBJECT
public:
    explicit PamBackend(HelperApp *parent);
    virtual ~PamBackend();
    int converse(int n, const struct pam_message **msg, struct pam_response **resp);

public slots:
    virtual bool start(const QString &user = QString());
    virtual bool authenticate();
    virtual bool openSession();
    virtual bool closeSession();

    virtual QString userName();

private:
    PamData *m_data{nullptr};
    PamHandle *m_pam{nullptr};
};
}

#endif // PAMBACKEND_H
