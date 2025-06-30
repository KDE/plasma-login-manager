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

#ifndef PLASMALOGIN_DISPLAYSERVER_H
#define PLASMALOGIN_DISPLAYSERVER_H

#include <QObject>

class QProcess;

namespace PLASMALOGIN
{
class Display;

class DisplayServer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DisplayServer)
public:
    explicit DisplayServer(Display *parent);

    Display *displayPtr() const;

    const QString &display() const;

    virtual QString sessionType() const = 0;

public slots:
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void finished() = 0;
    virtual void setupDisplay() = 0;

signals:
    void started();
    void stopped();

protected:
    bool m_started{false};

    QString m_display;

private:
    Display *m_displayPtr{nullptr};
};
}

#endif // PLASMALOGIN_DISPLAYSERVER_H
