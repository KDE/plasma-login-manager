/***************************************************************************
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

#ifndef PLASMALOGIN_XAUTH_H
#define PLASMALOGIN_XAUTH_H

#include <QString>
#include <QTemporaryFile>

namespace PLASMALOGIN {

class XAuth
{
public:
    XAuth();

    QString authDirectory() const;
    void setAuthDirectory(const QString &path);

    QString authPath() const;
    QByteArray cookie() const;

    void setup();
    bool addCookie(const QString &display);

    static bool writeCookieToFile(const QString &display,
                                  const QString &fileName,
                                  QByteArray cookie);

private:
    bool m_setup = false;
    QString m_authDir;
    QTemporaryFile m_authFile;
    QByteArray m_cookie;
};

} // namespace PLASMALOGIN

#endif // PLASMALOGIN_XAUTH_H
