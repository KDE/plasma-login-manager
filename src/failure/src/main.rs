// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

use std::fs;
use std::io;
use std::path::Path;

use gettextrs::{
    bind_textdomain_codeset, bindtextdomain, gettext, setlocale, textdomain, LocaleCategory,
};

const TEXTDOMAIN: &str = "plasma_login";
const DEFAULT_LOCALEDIR: &str = "/usr/share/locale";
const ISSUE_DIR: &str = "/run/issue.d";
const ISSUE_FILE: &str = "90-plasma-login-failure.issue";

fn main() -> io::Result<()> {
    // SAFETY: called once at startup, before any locale-sensitive work.
    unsafe { setlocale(LocaleCategory::LcAll, "") };

    bindtextdomain(TEXTDOMAIN, DEFAULT_LOCALEDIR)?;
    bind_textdomain_codeset(TEXTDOMAIN, "UTF-8")?;
    textdomain(TEXTDOMAIN)?;

    let os_release = rs_release::get_os_release().unwrap_or_default();
    let support_url = os_release.get("SUPPORT_URL").cloned().unwrap_or_default();
    let text = if support_url.is_empty() {
        gettext(
            "Plasma Login failed to start.
This usually indicates a severe problem with your system.
You can try restarting by pressing the power button to shut down.

You can get additional support from your distribution.",
        )
    } else {
        let text_with_support = gettext(
            "Plasma Login failed to start.
This usually indicates a severe problem with your system.
You can try restarting by pressing the power button to shut down.

Get additional support at: {support_url}",
        );
        text_with_support.replace("{support_url}", &support_url)
    };

    let width = text
        .lines()
        .map(|line| line.chars().count())
        .max()
        .unwrap_or(0);
    let text = text
        .lines()
        .map(|line| format!(" {line:<width$} "))
        .collect::<Vec<_>>()
        .join("\n");
    let pad = " ".repeat(width);

    let top = format!("┌{pad}┐");
    let bottom = format!("└{pad}┘");
    let content = format!("\n{top}\n{text}\n{bottom}\n\n");

    let dir = Path::new(ISSUE_DIR);
    fs::create_dir_all(dir)?;
    fs::write(dir.join(ISSUE_FILE), content)?;

    Ok(())
}
