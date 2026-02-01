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

#include "LogindDBusTypes.h"

#include <QDBusMetaType>

#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <QDebug>

class LogindPathInternal
{
public:
    LogindPathInternal();
    bool available = false;
};

LogindPathInternal::LogindPathInternal()
{
    qRegisterMetaType<NamedSeatPath>("NamedSeatPath");
    qDBusRegisterMetaType<NamedSeatPath>();

    qRegisterMetaType<NamedSeatPathList>("NamedSeatPathList");
    qDBusRegisterMetaType<NamedSeatPathList>();

    qRegisterMetaType<NamedSessionPath>("NamedSessionPath");
    qDBusRegisterMetaType<NamedSessionPath>();

    qRegisterMetaType<NamedSessionPathList>("NamedSessionPathList");
    qDBusRegisterMetaType<NamedSessionPathList>();

    qRegisterMetaType<SessionInfo>("SessionInfo");
    qDBusRegisterMetaType<SessionInfo>();

    qRegisterMetaType<SessionInfoList>("SessionInfoList");
    qDBusRegisterMetaType<SessionInfoList>();

    qRegisterMetaType<UserInfo>("UserInfo");
    qDBusRegisterMetaType<UserInfo>();

    qRegisterMetaType<UserInfoList>("UserInfoList");
    qDBusRegisterMetaType<UserInfoList>();

    if (QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.login1"))) {
        qDebug() << "Logind interface found";
        available = true;
        return;
    }

    qDebug() << "No session manager found";
}

Q_GLOBAL_STATIC(LogindPathInternal, s_instance);

bool Logind::isAvailable()
{
    return s_instance->available;
}
