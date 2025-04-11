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

#ifndef PLASMALOGIN_SESSION_H
#define PLASMALOGIN_SESSION_H

#include <QString>
#include <KSharedConfig>

namespace PLASMALOGIN {

class Session  {
public:
    enum Type {
        X11Session,
        WaylandSession
    };

    static Session create(Session::Type type, const QString &name);
    Session(); // creates an invalid session

    bool isValid() const;
    Type type() const;
    QString name() const;

    QString fileName() const;

    QString desktopSession() const;
    QString xdgSessionType() const;
    QString exec() const;
    QString desktopNames() const;

private:
    Session(Type type, KSharedConfigPtr desktopFile);
    Type m_type = WaylandSession;
    KSharedConfig::Ptr m_desktopFile;
    QString m_name;
};

    inline QDataStream &operator>>(QDataStream &stream, Session &session) {
        quint32 type;
        QString fileName;
        stream >> type >> fileName;
        session = Session::create(static_cast<Session::Type>(type), fileName);
        return stream;
    }



};

#endif // PLASMALOGIN_SESSION_H
