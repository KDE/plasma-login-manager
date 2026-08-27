// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#![feature(linux_pidfd)]

use std::{
    env,
    ffi::{CString, c_char, c_int, c_void},
    os::linux::process::CommandExt,
    path,
    process::{Command, Stdio},
};

mod bindings;
use bindings::*;

fn syslog(pam: *mut pam_handle_t, priority: u32, message: &str) {
    let c_message = CString::new(message).expect("Failed to convert message to CString");
    unsafe { pam_syslog(pam, priority as c_int, c_message.as_ptr()) };
}

fn set_item_authtok(pam: *mut pam_handle_t, authtok: &str) -> u32 {
    let c_authtok = CString::new(authtok).expect("Failed to convert authtok to CString");
    unsafe {
        if pam_set_item(
            pam,
            PAM_AUTHTOK as c_int,
            c_authtok.as_ptr() as *const c_void,
        ) != PAM_SUCCESS as c_int
        {
            syslog(pam, LOG_ERR, "Failed to set PAM_AUTHTOK");
            return PAM_SERVICE_ERR;
        }
    }
    PAM_SUCCESS
}

fn sm_open_session(pam: *mut pam_handle_t) -> u32 {
    // systemd doesn't set CREDENTIALS_DIRECTORY for PAM so we need to hardcode /run/credentials :(
    let unit = match env::var("SYSTEMD_ACTIVATION_UNIT") {
        Ok(val) => val,
        Err(_) => {
            syslog(pam, LOG_ERR, "SYSTEMD_ACTIVATION_UNIT is not set");
            return PAM_SERVICE_ERR;
        }
    };

    let credential_path = path::Path::new("/run/credentials")
        .join(&unit)
        .join("encrypted.system.pam.authtok.plasmalogin");
    let credential_path_str = credential_path
        .into_string()
        .expect("Failed to convert credential path to String");

    let creds = Command::new("/usr/bin/systemd-creds")
        .args([
            "decrypt",
            &credential_path_str,
            "-", // output to stdout
        ])
        .create_pidfd(true)
        .stdout(Stdio::piped())
        .spawn()
        .expect("Failed to spawn child");

    let password = match creds.wait_with_output() {
        Ok(output) => {
            if !output.status.success() {
                syslog(pam, LOG_ERR, "Failed to decrypt credentials");
                return PAM_SERVICE_ERR;
            }
            String::from_utf8_lossy(&output.stdout).trim().to_string()
        }
        Err(_) => {
            syslog(pam, LOG_ERR, "Failed to wait on child process");
            return PAM_SERVICE_ERR;
        }
    };

    if set_item_authtok(pam, &password) != PAM_SUCCESS {
        syslog(pam, LOG_ERR, "pam_set_item(PAM_AUTHTOK) failed");
        return PAM_SERVICE_ERR;
    }

    return PAM_SUCCESS;
}

#[unsafe(no_mangle)] // We need this exact symbol name!
pub unsafe extern "C" fn pam_sm_open_session(
    pam: *mut pam_handle_t,
    _flags: c_int,
    _argc: c_int,
    _argv: *mut *const c_char,
) -> c_int {
    return sm_open_session(pam) as c_int;
}

#[unsafe(no_mangle)] // We need this exact symbol name!
pub unsafe extern "C" fn pam_sm_close_session(
    _pam: *mut pam_handle_t,
    _flags: c_int,
    _argc: c_int,
    _argv: *mut *const c_char,
) -> c_int {
    return PAM_SUCCESS as c_int;
}
