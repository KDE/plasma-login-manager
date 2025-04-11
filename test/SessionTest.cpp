/***************************************************************************
* SPDX-FileCopyrightText: 2023 Fabian Vogt <fvogt@suse.de>
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

#include "Session.h"

#include <QLocale>
#include <QTest>

class SessionTest : public QObject {
    Q_OBJECT
private slots:
    void testCLocale()
    {
        QLocale::setDefault(QLocale::c());
        auto fileName = QFINDTESTDATA("plasmawayland-dev.desktop");
        PLASMALOGIN::Session session(PLASMALOGIN::Session::WaylandSession, fileName);
        QVERIFY(session.isValid());
        QCOMPARE(session.xdgSessionType(), QStringLiteral("wayland"));
        QCOMPARE(session.fileName(), fileName);
        QCOMPARE(session.displayName(), QStringLiteral("Plasma (Development, Wayland /usr/bin)"));
        QCOMPARE(session.comment(), QStringLiteral("Plasma by KDE"));
        QCOMPARE(session.exec(), QStringLiteral("/usr/lib64/libexec/plasma-dbus-run-session-if-needed /usr/lib64/libexec/startplasma-dev.sh -wayland"));
        QCOMPARE(session.tryExec(), QString());
        QCOMPARE(session.desktopSession(), QStringLiteral("plasmawayland-dev"));
        QCOMPARE(session.desktopNames(), QStringLiteral("KDE"));
        QCOMPARE(session.isHidden(), false);
        QCOMPARE(session.isNoDisplay(), false);
    }
    void testKOLocale()
    {
        QLocale::setDefault(QLocale{QStringLiteral("ko_KO")});
        auto fileName = QFINDTESTDATA("plasmawayland-dev.desktop");
        PLASMALOGIN::Session session(PLASMALOGIN::Session::WaylandSession, fileName);
        QVERIFY(session.isValid());
        QCOMPARE(session.xdgSessionType(), QStringLiteral("wayland"));
        QCOMPARE(session.fileName(), fileName);
        QCOMPARE(session.displayName(), QStringLiteral("Plasma(\uAC1C\uBC1C, Wayland /usr/bin)"));
        QCOMPARE(session.comment(), QStringLiteral("KDE Plasma"));
        QCOMPARE(session.exec(), QStringLiteral("/usr/lib64/libexec/plasma-dbus-run-session-if-needed /usr/lib64/libexec/startplasma-dev.sh -wayland"));
        QCOMPARE(session.tryExec(), QString());
        QCOMPARE(session.desktopSession(), QStringLiteral("plasmawayland-dev"));
        QCOMPARE(session.desktopNames(), QStringLiteral("KDE"));
        QCOMPARE(session.isHidden(), false);
        QCOMPARE(session.isNoDisplay(), false);
    }
};

QTEST_MAIN(SessionTest);

#include "SessionTest.moc"
