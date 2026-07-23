/***************************************************************************
 * SPDX-FileCopyrightText: 2014 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 * SPDX-FileCopyrightText: 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ***************************************************************************/

#include "Seat.h"

#include "DaemonApp.h"
#include "MainConfigLoader.h"
#include "VirtualTerminal.h"

#include <QDebug>
#include <QFile>
#include <QTimer>

#include "Constants.h"
#include "config.h"
#include <KConfig>
#include <KSharedConfig>
#include <QDir>
#include <QFileInfo>

#include <Login1Manager.h>
#include <Login1Seat.h>
#include <Login1Session.h>
#include <functional>
#include <optional>
#include <unistd.h>

namespace PLASMALOGIN
{
Seat::Seat(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
    createDisplay();
}

const QString &Seat::name() const
{
    return m_name;
}

bool Seat::isTtyInUse(const QString &tty) const
{
    if (!Logind::isAvailable()) {
        return false;
    }

    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    auto reply = manager.ListSessions();
    reply.waitForFinished();

    const auto info = reply.value();
    for (const SessionInfo &sessionInfo : info) {
        OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), sessionInfo.sessionPath.path(), QDBusConnection::systemBus());
        if (tty == session.tTY() && session.state() != QLatin1String("closing")) {
            qDebug() << "tty" << tty << "already in use by" << session.user().path.path() << session.state() << session.display() << session.desktop()
                     << session.vTNr();
            return true;
        }
    }

    return false;
}

int Seat::availableVt() const
{
    if (!isTtyInUse(QStringLiteral("tty%1").arg(PLASMALOGIN_INITIAL_VT))) {
        return PLASMALOGIN_INITIAL_VT;
    }

    const auto vt = VirtualTerminal::currentVt();
    if (vt > 0 && !isTtyInUse(QStringLiteral("tty%1").arg(vt))) {
        return vt;
    }

    return VirtualTerminal::setUpNewVt();
}

QString Seat::reusableSessionId(const QString &user) const
{
    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    auto reply = manager.ListSessions();
    reply.waitForFinished();

    for (const SessionInfo &sessionInfo : reply.value()) {
        if (sessionInfo.userName != user) {
            continue;
        }

        OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), sessionInfo.sessionPath.path(), QDBusConnection::systemBus());
        if (session.service() == QLatin1String("plasmalogin") && session.state() == QLatin1String("online")) {
            return sessionInfo.sessionId;
        }
    }

    return {};
}

void Seat::activateSession(const QString &sessionId) const
{
    if (sessionId.isEmpty()) {
        return;
    }

    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    manager.UnlockSession(sessionId);
    manager.ActivateSession(sessionId);
}

std::optional<int> Seat::vtForSession(const QString &sessionId) const
{
    if (sessionId.isEmpty()) {
        return std::nullopt;
    }

    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    if (!manager.isValid()) {
        return std::nullopt;
    }

    auto sessionPath = manager.GetSession(sessionId);
    OrgFreedesktopLogin1SessionInterface session(Logind::serviceName(), sessionPath.value().path(), QDBusConnection::systemBus());
    return QStringView(session.tTY()).mid(3).toInt(); // we need to convert ttyN to N
}

void Seat::createDisplay()
{
    PlasmaLogin::config()->load();

    // create a new display
    qDebug() << "Adding new display...";
    Display *display = new Display(this);

    // restart display on stop
    connect(display, &Display::stopped, this, &Seat::displayStopped);

    // add display to the list
    m_displays << display;

    // Per-seat autologin overrides the global [Autologin] keys for a dedicated seat.
    // Resolve it here, after the configuration has been reloaded, rather than caching it
    // in Display's constructor.
    QString autologinUser;
    QString autologinSession;
    const bool firstLogin = tryLockFirstLogin();
    const KConfigGroup autologinGroup = PlasmaLogin::config()->config()->group(QStringLiteral("Autologin"));
    if (autologinGroup.hasGroup(m_name)) {
        const KConfigGroup seatGroup = autologinGroup.group(m_name);
        if (seatGroup.readEntry("Relogin", PlasmaLogin::config()->autologinRelogin()) || firstLogin) {
            autologinUser = seatGroup.readEntry("User", QString());
            autologinSession = seatGroup.readEntry("Session", QString());
            if (autologinUser.isEmpty()) {
                qWarning() << "Per-seat autologin: seat" << m_name << "is configured to autologin but names no User; it will be greeted.";
            } else {
                qInfo() << "Per-seat autologin: seat" << m_name << "configured for user" << autologinUser << "session" << autologinSession;
            }
        } else {
            qDebug() << "Per-seat autologin: seat" << m_name << "has a config subgroup but Relogin is off and this is not the first login; it will be greeted.";
        }
    } else if (PlasmaLogin::config()->autologinRelogin() || firstLogin) {
        autologinUser = PlasmaLogin::config()->autologinUser();
        autologinSession = PlasmaLogin::config()->autologinSession();
    }
    if (auto *autologinDisplay = qobject_cast<AutoLoginDisplay *>(display)) {
        autologinDisplay->setAutoLogin(autologinUser, autologinSession);
    }

    // start the display
    startDisplay(display);
}

void Seat::startDisplay(Display *display, int tryNr)
{
    if (display->start()) {
        return;
    }

    // It's possible that the system isn't ready yet (driver not loaded,
    // device not enumerated, ...). It's not possible to tell when that changes,
    // so try a few times with a delay in between.
    qWarning() << "Attempt" << tryNr << "starting the Display server on vt" << display->terminalId() << "failed";

    if (tryNr >= 3) {
        qCritical() << "Could not start Display server on vt" << display->terminalId();
        return;
    }

    QTimer::singleShot(2000, display, [this, display, tryNr] {
        startDisplay(display, tryNr + 1);
    });
}

void Seat::displayStopped()
{
    Display *display = qobject_cast<Display *>(sender());
    std::optional<int> nextVt;
    nextVt = vtForSession(display->reuseSessionId());

    // remove display from list
    m_displays.removeAll(display);
    // delete display
    display->deleteLater();

    // restart otherwise
    if (m_displays.isEmpty()) {
        createDisplay();
    }
    // If there is still a session running on some display,
    // switch to last display in display vector.
    // Set vt_auto to true, so let the kernel handle the
    // vt switch automatically (VT_AUTO).
    else if (!nextVt) {
        int disp = m_displays.last()->terminalId();
        if (disp != -1) {
            nextVt = disp;
        }
    }

    if (nextVt) {
        VirtualTerminal::jumpToVt(*nextVt, true);
    }
}

bool Seat::canTTY()
{
    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    if (manager.isValid()) {
        auto seatPath = manager.GetSeat(m_name);
        OrgFreedesktopLogin1SeatInterface seatIface(Logind::serviceName(), seatPath.value().path(), QDBusConnection::systemBus());
        if (seatIface.property("CanTTY").isValid()) {
            return seatIface.canTTY();
        }
    }

    return m_name.compare(QStringLiteral("seat0"), Qt::CaseInsensitive) == 0 && access(VirtualTerminal::defaultVtPath, F_OK) == 0;
}

bool Seat::tryLockFirstLogin()
{
    // One first-login token per seat, so each seat independently gets its
    // first-boot login; the machine-global soft-reboot check stays on the
    // daemon (isFirstBoot).
    if (m_firstLoginLock) {
        return false;
    }
    m_firstLoginLock = true;
    return daemonApp->isFirstBoot();
}
}

#include "moc_Seat.cpp"
