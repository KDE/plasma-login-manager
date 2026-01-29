/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KScreenDpms/Dpms>
#include <kscreendpms/dpms.h>

#include "greetereventfilter.h"

GreeterEventFilter::GreeterEventFilter(QObject *parent)
    : QObject(parent)
{
}

bool GreeterEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Escape) {
            // Esc -> turn off screens
            auto dpms = new KScreen::Dpms(this);
            if (dpms->isSupported()) {
                connect(dpms, &KScreen::Dpms::hasPendingChangesChanged, this, [dpms](bool hasPendingChanges) {
                    if (!hasPendingChanges) {
                        dpms->deleteLater();
                    }
                });
                dpms->switchMode(KScreen::Dpms::Off);
            } else {
                dpms->deleteLater();
            }

            Q_EMIT escapeKeyPressed();

            return true;
        } else {
            // Any other key -> wake greeter
            Q_EMIT keyPressed();
        }
    }

    return false;
}
