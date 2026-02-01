/***************************************************************************
 * SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>
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

#ifndef LOGIND_DBUSTYPES
#define LOGIND_DBUSTYPES

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QString>

struct Logind {
    static bool isAvailable();

    // These identifiers are constants; no need to resolve at runtime
    static inline QString serviceName()
    {
        return QStringLiteral("org.freedesktop.login1");
    }
    static inline QString managerPath()
    {
        return QStringLiteral("/org/freedesktop/login1");
    }
    static inline QString seatIfaceName()
    {
        return QStringLiteral("org.freedesktop.login1.Seat");
    }
};

struct SessionInfo {
    QString sessionId;
    uint userId;
    QString userName;
    QString seatId;
    QDBusObjectPath sessionPath;
};

typedef QList<SessionInfo> SessionInfoList;

inline QDBusArgument &operator<<(QDBusArgument &argument, const SessionInfo &sessionInfo)
{
    argument.beginStructure();
    argument << sessionInfo.sessionId;
    argument << sessionInfo.userId;
    argument << sessionInfo.userName;
    argument << sessionInfo.seatId;
    argument << sessionInfo.sessionPath;
    argument.endStructure();

    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, SessionInfo &sessionInfo)
{
    argument.beginStructure();
    argument >> sessionInfo.sessionId;
    argument >> sessionInfo.userId;
    argument >> sessionInfo.userName;
    argument >> sessionInfo.seatId;
    argument >> sessionInfo.sessionPath;
    argument.endStructure();

    return argument;
}

struct UserInfo {
    uint userId;
    QString name;
    QDBusObjectPath path;
};

typedef QList<UserInfo> UserInfoList;

inline QDBusArgument &operator<<(QDBusArgument &argument, const UserInfo &userInfo)
{
    argument.beginStructure();
    argument << userInfo.userId;
    argument << userInfo.name;
    argument << userInfo.path;
    argument.endStructure();

    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, UserInfo &userInfo)
{
    argument.beginStructure();
    argument >> userInfo.userId;
    argument >> userInfo.name;
    argument >> userInfo.path;
    argument.endStructure();

    return argument;
}

struct NamedSeatPath {
    QString name;
    QDBusObjectPath path;
};

inline QDBusArgument &operator<<(QDBusArgument &argument, const NamedSeatPath &namedSeat)
{
    argument.beginStructure();
    argument << namedSeat.name;
    argument << namedSeat.path;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, NamedSeatPath &namedSeat)
{
    argument.beginStructure();
    argument >> namedSeat.name;
    argument >> namedSeat.path;
    argument.endStructure();
    return argument;
}

typedef QList<NamedSeatPath> NamedSeatPathList;

typedef NamedSeatPath NamedSessionPath;
typedef NamedSeatPathList NamedSessionPathList;

class NamedUserPath
{
public:
    uint userId;
    QDBusObjectPath path;
};

inline QDBusArgument &operator<<(QDBusArgument &argument, const NamedUserPath &namedUser)
{
    argument.beginStructure();
    argument << namedUser.userId;
    argument << namedUser.path;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, NamedUserPath &namedUser)
{
    argument.beginStructure();
    argument >> namedUser.userId;
    argument >> namedUser.path;
    argument.endStructure();
    return argument;
}

class Inhibitor
{
public:
    QString what;
    QString who;
    QString why;
    QString mode;
    int userId;
    uint processId;
};

typedef QList<Inhibitor> InhibitorList;

inline QDBusArgument &operator<<(QDBusArgument &argument, const Inhibitor &inhibitor)
{
    argument.beginStructure();
    argument << inhibitor.what;
    argument << inhibitor.who;
    argument << inhibitor.why;
    argument << inhibitor.mode;
    argument << inhibitor.userId;
    argument << inhibitor.processId;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, Inhibitor &inhibitor)
{
    argument.beginStructure();
    argument >> inhibitor.what;
    argument >> inhibitor.who;
    argument >> inhibitor.why;
    argument >> inhibitor.mode;
    argument >> inhibitor.userId;
    argument >> inhibitor.processId;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(SessionInfo);
Q_DECLARE_METATYPE(QList<SessionInfo>);

Q_DECLARE_METATYPE(UserInfo);
Q_DECLARE_METATYPE(QList<UserInfo>);

Q_DECLARE_METATYPE(NamedSeatPath);
Q_DECLARE_METATYPE(QList<NamedSeatPath>);

Q_DECLARE_METATYPE(NamedUserPath);

Q_DECLARE_METATYPE(Inhibitor);
Q_DECLARE_METATYPE(QList<Inhibitor>);

#endif
