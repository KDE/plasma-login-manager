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

#ifndef PLASMALOGIN_SEAT_H
#define PLASMALOGIN_SEAT_H

#include <QObject>
#include <QVector>
#include "Display.h"

namespace PLASMALOGIN {
    class Display;

    class Seat : public QObject {
        Q_OBJECT
        Q_DISABLE_COPY(Seat)
    public:
        explicit Seat(const QString &name, QObject *parent = 0);

        const QString &name() const;
        void createDisplay();
        bool canTTY();

    public slots:
        void removeDisplay(PLASMALOGIN::Display* display);

    private slots:
        void displayStopped();

    private:
        void startDisplay(PLASMALOGIN::Display *display, int tryNr = 1);

        QString m_name;

        QVector<Display *> m_displays;
    };
}

#endif // PLASMALOGIN_SEAT_H
