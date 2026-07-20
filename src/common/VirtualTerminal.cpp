/***************************************************************************
 * SPDX-FileCopyrightText: 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#include <QDebug>

#include "VirtualTerminal.h"

#include <QFileInfo>
#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <qscopeguard.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define RELEASE_DISPLAY_SIGNAL (SIGRTMAX)
#define ACQUIRE_DISPLAY_SIGNAL (SIGRTMAX - 1)

namespace PLASMALOGIN
{
namespace VirtualTerminal
{
const char *defaultVtPath = "/dev/tty0";

QString path(int vt)
{
    return QStringLiteral("/dev/tty%1").arg(vt);
}

int getVtActive(int fd)
{
    vt_stat vtState{};
    if (ioctl(fd, VT_GETSTATE, &vtState) < 0) {
        qCritical() << "Failed to get current VT:" << strerror(errno);
        return -1;
    }
    return vtState.v_active;
}

static void onAcquireDisplay([[maybe_unused]] int signal)
{
    int fd = open(defaultVtPath, O_RDWR | O_NOCTTY);
    ioctl(fd, VT_RELDISP, VT_ACKACQ);
    close(fd);
}

static void onReleaseDisplay([[maybe_unused]] int signal)
{
    int fd = open(defaultVtPath, O_RDWR | O_NOCTTY);
    ioctl(fd, VT_RELDISP, 1);
    close(fd);
}

static bool handleVtSwitches(int fd)
{
    vt_mode setModeRequest{};
    bool ok = true;

    setModeRequest.mode = VT_PROCESS;
    setModeRequest.relsig = RELEASE_DISPLAY_SIGNAL;
    setModeRequest.acqsig = ACQUIRE_DISPLAY_SIGNAL;

    if (ioctl(fd, VT_SETMODE, &setModeRequest) < 0) {
        qDebug() << "Failed to manage VT manually:" << strerror(errno);
        ok = false;
    }

    signal(RELEASE_DISPLAY_SIGNAL, onReleaseDisplay);
    signal(ACQUIRE_DISPLAY_SIGNAL, onAcquireDisplay);

    return ok;
}

int currentVt()
{
    int fd = open(defaultVtPath, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        qCritical() << "Failed to open VT master:" << strerror(errno);
        return -1;
    }
    auto closeFd = qScopeGuard([fd] {
        close(fd);
    });

    return getVtActive(fd);
}

int setUpNewVt()
{
    // open VT master
    int fd = open(defaultVtPath, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        qCritical() << "Failed to open VT master:" << strerror(errno);
        return -1;
    }
    auto closeFd = qScopeGuard([fd] {
        close(fd);
    });

    int vt = 0;
    if (ioctl(fd, VT_OPENQRY, &vt) < 0) {
        qCritical() << "Failed to open new VT:" << strerror(errno);
        return -1;
    }

    // fallback to active VT
    if (vt <= 0) {
        int vtActive = getVtActive(fd);
        qWarning() << "New VT" << vt << "is not valid, fall back to" << vtActive;
        return vtActive;
    }

    return vt;
}

void jumpToVt(int vt, bool vt_auto)
{
    qDebug() << "Jumping to VT" << vt;

    int fd;

    int activeVtFd = open(defaultVtPath, O_RDWR | O_NOCTTY);

    QString ttyString = path(vt);
    int vtFd = open(qPrintable(ttyString), O_RDWR | O_NOCTTY);
    if (vtFd != -1) {
        fd = vtFd;

        // Clear VT
        static const char *clearEscapeSequence = "\33[H\33[2J";
        if (write(vtFd, clearEscapeSequence, sizeof(clearEscapeSequence)) == -1) {
            qWarning("Failed to clear VT %d: %s", vt, strerror(errno));
        }

    } else {
        qWarning("Failed to open %s: %s", qPrintable(ttyString), strerror(errno));
        qDebug("Using %s instead of %s!", defaultVtPath, qPrintable(ttyString));
        fd = activeVtFd;
    }

    // If vt_auto is true, the controlling process is already gone, so there is no
    // process which could send the VT_RELDISP 1 ioctl to release the vt.
    // Let the kernel switch vts automatically
    if (!vt_auto) {
        handleVtSwitches(fd);
    }

    do {
        errno = 0;

        if (ioctl(fd, VT_ACTIVATE, vt) < 0) {
            if (errno == EINTR) {
                continue;
            }

            qWarning("Couldn't initiate jump to VT %d: %s", vt, strerror(errno));
            break;
        }

        if (ioctl(fd, VT_WAITACTIVE, vt) < 0 && errno != EINTR) {
            qWarning("Couldn't finalize jump to VT %d: %s", vt, strerror(errno));
        }

    } while (errno == EINTR);
    close(activeVtFd);
    if (vtFd != -1) {
        close(vtFd);
    }
}
}
}
