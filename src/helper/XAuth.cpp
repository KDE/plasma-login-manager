/***************************************************************************
 * SPDX-FileCopyrightText: 2023 Fabian Vogt <fvogt@suse.de>
 * SPDX-FileCopyrightText: 2021 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#include <QDebug>
#include <QDir>
#include <QScopeGuard>
#include <QStringView>
#include <X11/Xauth.h>
#include <limits.h>
#include <random>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Constants.h"
#include "XAuth.h"

namespace PLASMALOGIN
{

XAuth::XAuth()
{
    m_authDir = QStringLiteral(RUNTIME_DIR);
}

QString XAuth::authDirectory() const
{
    return m_authDir;
}

void XAuth::setAuthDirectory(const QString &path)
{
    if (m_setup) {
        qWarning("Unable to set xauth directory after setup");
        return;
    }

    m_authDir = path;
}

QString XAuth::authPath() const
{
    return m_authFile.fileName();
}

QByteArray XAuth::cookie() const
{
    return m_cookie;
}

void XAuth::setup()
{
    if (m_setup) {
        return;
    }

    m_setup = true;

    // Create directory if not existing
    QDir().mkpath(m_authDir);

    // Set path
    m_authFile.setFileTemplate(m_authDir + QStringLiteral("/xauth_XXXXXX"));
    if (!m_authFile.open()) {
        qFatal("Failed to create xauth file");
    }

    qDebug() << "Xauthority path:" << authPath();

    // Generate cookie
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 0xFF);

    m_cookie.truncate(0);
    m_cookie.reserve(16);

    // Create a random hexadecimal number
    for (int i = 0; i < 16; i++) {
        m_cookie.append(dis(gen));
    }
}

bool XAuth::addCookie(const QString &display)
{
    if (!m_setup) {
        qWarning("Please setup xauth before adding a cookie");
        return false;
    }

    return XAuth::writeCookieToFile(display, authPath(), m_cookie);
}

bool XAuth::writeCookieToFile(const QString &display, const QString &fileName, QByteArray cookie)
{
    qDebug() << "Writing cookie to" << fileName;

    if (display.size() < 2 || display[0] != QLatin1Char(':') || cookie.size() != 16) {
        qWarning().nospace() << "Unexpected DISPLAY='" << display << "' or cookie.size() = " << cookie.size();
        return false;
    }

    // The file needs 0600 permissions
    const int oldumask = umask(077);

    // Truncate the file. We don't support merging like the xauth tool does.
    FILE *const authFp = fopen(qPrintable(fileName), "wb");
    auto error = errno;
    umask(oldumask);
    if (authFp == nullptr) {
        qWarning().nospace() << "fopen() failed with errno=" << error;
        return false;
    }

    auto fileCloser = qScopeGuard([authFp] {
        fclose(authFp);
    });
    char localhost[HOST_NAME_MAX + 1] = "";
    if (gethostname(localhost, sizeof(localhost)) < 0) {
        strcpy(localhost, "localhost");
    }

    ::Xauth auth = {};
    char cookieName[] = "MIT-MAGIC-COOKIE-1";

    // Skip the ':'
    QByteArray displayNumberUtf8 = QStringView{display}.mid(1).toUtf8();

    auth.family = FamilyLocal;
    auth.address = localhost;
    auth.address_length = strlen(auth.address);
    auth.number = displayNumberUtf8.data();
    auth.number_length = displayNumberUtf8.size();
    auth.name = cookieName;
    auth.name_length = sizeof(cookieName) - 1;
    auth.data = cookie.data();
    auth.data_length = cookie.size();

    errno = 0;
    if (XauWriteAuth(authFp, &auth) == 0) {
        qWarning().nospace() << "XauWriteAuth(FamilyLocal) failed with errno=" << errno;
        return false;
    }

    // Write the same entry again, just with FamilyWild
    auth.family = FamilyWild;
    auth.address_length = 0;
    errno = 0;
    if (XauWriteAuth(authFp, &auth) == 0) {
        qWarning().nospace() << "XauWriteAuth(FamilyWild) failed with errno=" << errno;
        return false;
    }

    if (fflush(authFp) != 0) {
        qWarning().nospace() << "fflush() failed with errno=" << errno;
        return false;
    }

    return true;
}

} // namespace PLASMALOGIN
