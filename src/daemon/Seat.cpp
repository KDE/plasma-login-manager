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

#include "config.h"
#include <KConfig>
#include <KSharedConfig>
#include <QDir>
#include <QFileInfo>

#include <Login1Manager.h>
#include <Login1Seat.h>
#include <Login1Session.h>
#include <functional>
#include <unistd.h>

using namespace Qt::StringLiterals;

namespace
{

struct Autologin {
    bool relogin;
    QString user;
    QString session;
};

} // namespace

namespace PLASMALOGIN
{
Seat::Seat(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
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

VirtualTerminal::Terminal Seat::availableVt() const
{
    if (!isTtyInUse(QStringLiteral("tty%1").arg(PLASMALOGIN_INITIAL_VT))) {
        return VirtualTerminal::openVt(PLASMALOGIN_INITIAL_VT);
    }

    const auto vt = VirtualTerminal::currentVt();
    if (vt > 0 && !isTtyInUse(QStringLiteral("tty%1").arg(vt))) {
        return VirtualTerminal::openVt(vt);
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
        qInfo() << "Checking session:" << sessionInfo.sessionId << "for user:" << user << "with service:" << session.service()
                << "and state:" << session.state();
        // Be careful when making changes here as the logic is a bit tricky.
        // A session may be reused if it is from us and it is either online (not in the foreground) or active (in the foreground).
        // The active part may be confusing... When switching user from an autologin session **with relogin** we are so fast that the session may still be
        // marked active! No harm though, we can still reuse it just fine (we'll basically do a no-op activate run).
        if ((session.service() == QLatin1String("plasmalogin") || session.service() == QLatin1String("plasmalogin-autologin"))
            && (session.state() == QLatin1String("online") || session.state() == QLatin1String("active"))) {
            return sessionInfo.sessionId;
        }
    }

    return {};
}

void Seat::activateSession(const QString &sessionId) const
{
    qDebug() << "Activating session with ID:" << sessionId;
    if (sessionId.isEmpty()) {
        qWarning() << "No session ID provided to activate.";
        return;
    }

    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    manager.ActivateSession(sessionId);
}

void Seat::unlockAndActivateSession(const QString &sessionId) const
{
    qDebug() << "Unlocking and activating session with ID:" << sessionId;
    if (sessionId.isEmpty()) {
        qWarning() << "No session ID provided to unlock and activate.";
        return;
    }

    OrgFreedesktopLogin1ManagerInterface manager(Logind::serviceName(), Logind::managerPath(), QDBusConnection::systemBus());
    manager.UnlockSession(sessionId);
    manager.ActivateSession(sessionId);
}

bool Seat::activateExistingGreeter()
{
    if (!m_greeterSession) {
        qDebug() << "No existing greeter session found. Not able to activate it.";
        return false;
    }

    qDebug() << "Activating existing greeter session...";
    m_greeterSession->activate();
    return true;
}

void Seat::switchToGreeter()
{
    // A greeter becomes visible after this. UNLESS there is something super wrong with our state machine!
    if (!activateExistingGreeter()) {
        auto display = createDisplay();
        if (!display) {
            return;
        }
        maybeSetupAutoLogin(display);
        display->start();
    }
}

void Seat::setGreeterSession(const std::optional<QDBusObjectPath> &objectPath)
{
    if (!objectPath) {
        qInfo() << "Resetting greeter session for seat:" << m_name;
        m_greeterSession.reset();
        return;
    }

    if (m_greeterSession) {
        qWarning() << "Greeter session already exists, replacing it. This is probably a bug!";
    }
    m_greeterSession.emplace(objectPath.value());
}

Display *Seat::createDisplay()
{
    // Check if we have a display "queued". This is important to not double-queue startup of greeter displays.
    for (const auto &display : m_displays) {
        if (display->isGreeter()) {
            qWarning() << "Found an existing display in greeter state, reusing it. This may indicate a race condition and should get investigated!";
            return display;
        }
    }
    // If no existing display is in greeter state then we need a new one.

    qDebug() << "Adding new display...";
    Display *display = new Display(this);

    // restart display on stop
    connect(display, &Display::stopped, this, &Seat::displayStopped);

    // add display to the list
    m_displays << display;

    return display;
}

void Seat::maybeSetupAutoLogin(Display *display)
{
    PlasmaLogin::config()->load();

    // Per-seat autologin overrides the global [Autologin] keys for a dedicated seat.
    // Resolve it here, after the configuration has been reloaded, rather than caching it
    // in Display's constructor.

    const bool firstLogin = tryLockFirstLogin();

    // The global autologin configuration is used as a fallback if the seat-specific one is not set.
    auto globalAutologin = []() -> Autologin {
        auto autologinGroup = PlasmaLogin::config()->config()->group(u"Autologin"_s);
        return Autologin{
            .relogin = PlasmaLogin::config()->autologinRelogin(),
            .user = PlasmaLogin::config()->autologinUser(),
            .session = PlasmaLogin::config()->autologinSession(),
        };
    }();

    // The effective configuration is the seat-specific, falling back to global as necessary.
    auto effectiveAutologin = [this, &globalAutologin]() -> Autologin {
        auto autologinGroup = PlasmaLogin::config()->config()->group(u"Autologin"_s);
        auto group = autologinGroup.group(m_name);
        return Autologin{
            .relogin = group.readEntry("Relogin", globalAutologin.relogin),
            .user = group.readEntry("User", globalAutologin.user),
            .session = group.readEntry("Session", globalAutologin.session),
        };
    }();

    qDebug() << "Effective autologin configuration for seat" << m_name << ":" << "relogin=" << effectiveAutologin.relogin << "user=" << effectiveAutologin.user
             << "session=" << effectiveAutologin.session << ". isfirstLogin=" << firstLogin;

    auto allowed = firstLogin || effectiveAutologin.relogin;
    if (!allowed) {
        return;
    }
    display->setAutoLogin(effectiveAutologin.user, effectiveAutologin.session);
}

void Seat::displayStopped()
{
    Display *display = qobject_cast<Display *>(sender());
    if (!display) {
        return;
    }

    // remove display from list
    m_displays.removeAll(display);
    // delete display
    display->deleteLater();
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
