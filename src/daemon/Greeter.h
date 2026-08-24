/***************************************************************************
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

#ifndef PLASMALOGIN_GREETER_H
#define PLASMALOGIN_GREETER_H

#include <QObject>

#include "Auth.h"

class QProcess;

namespace PLASMALOGIN
{
class Display;

class Greeter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Greeter)
public:
    explicit Greeter(Display *parent = 0);
    ~Greeter();

    void setSocket(const QString &socket);
    bool isRunning() const;

public slots:
    bool start();
    void stop();

private slots:
    void onHelperFinished(Auth::HelperExitStatus status);

signals:
    void ttyFailed();
    void failed();
    void displayServerFailed();

private:
    Display *const m_display{nullptr};
    QString m_socket;
    QProcess *m_process{nullptr};
    QString m_unitName;

    static void insertEnvironmentList(QStringList names, QProcessEnvironment sourceEnv, QProcessEnvironment &targetEnv);
};
}

#endif // PLASMALOGIN_GREETER_H
