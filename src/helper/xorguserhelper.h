/***************************************************************************
* SPDX-FileCopyrightText: 2021 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#ifndef XORGUSERHELPER_H
#define XORGUSERHELPER_H

#include <QProcess>

#include "XAuth.h"

namespace PLASMALOGIN {

class XOrgUserHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString display READ display NOTIFY displayChanged)
public:
    explicit XOrgUserHelper(QObject *parent = nullptr);

    /// @returns the system environment plus the variables we need here
    QProcessEnvironment sessionEnvironment() const;

    QString display() const;

    bool start(const QString &cmd);
    void stop();

Q_SIGNALS:
    void displayChanged(const QString &display);

private:
    QString m_display = QStringLiteral(":0");
    XAuth m_xauth;
    QProcess *m_serverProcess = nullptr;

    bool startProcess(const QString &cmd, const QProcessEnvironment &env,
                      QProcess **p = nullptr);
    bool startServer(const QString &cmd);
    void startDisplayCommand();
    void displayFinished();
};

} // namespace PLASMALOGIN

#endif // XORGUSERHELPER_H
