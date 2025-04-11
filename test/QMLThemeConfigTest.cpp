/***************************************************************************
* SPDX-FileCopyrightText: 2023 Fabian Vogt <fabian@ritter-vogt.de>
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

#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

#include "ThemeConfig.h"

class Setup : public QObject
{
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        auto *config = new PLASMALOGIN::ThemeConfig(QStringLiteral("theme.conf"), this);
        engine->rootContext()->setContextProperty(QStringLiteral("config"), config);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(QMLThemeConfigTest, Setup)

#include "QMLThemeConfigTest.moc"
