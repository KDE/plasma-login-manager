// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include <security/pam_appl.h>
#include <security/pam_ext.h>
#include <security/pam_modules.h>

#include <cassert>
#include <iostream>

#include <QProcess>
#include <sys/syslog.h>

using namespace Qt::StringLiterals;

Q_DECL_EXPORT PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    assert(pamh);

    // systemd doesn't set CREDENTIALS_DIRECTORY for PAM so we need to hardcode /run/credentials :(
    QString credentialFile = u"/run/credentials/"_s + qEnvironmentVariable("SYSTEMD_ACTIVATION_UNIT") + u"/encrypted.system.pam.authtok.plasmalogin"_s;
    std::cerr << "Looking for credentials in " << credentialFile.toStdString() << '\n';

    QProcess creds;
    creds.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    creds.setProgram("systemd-creds");
    creds.setArguments({
        u"decrypt"_s,
        credentialFile, // input
        u"-"_s, // stdout
    });
    creds.start();
    creds.waitForFinished();
    auto password = creds.readAllStandardOutput().trimmed();

    if (auto ret = pam_set_item(pamh, PAM_AUTHTOK, password.constData()); ret != PAM_SUCCESS) {
        pam_syslog(pamh, LOG_ERR, "pam_set_item(PAM_AUTHTOK) failed: %d", ret);
        return PAM_SERVICE_ERR;
    }

    return PAM_SUCCESS;
}

Q_DECL_EXPORT PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return PAM_SUCCESS;
}
