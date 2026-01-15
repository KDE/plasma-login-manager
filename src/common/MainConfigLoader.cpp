/***************************************************************************
 * SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
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

#include "MainConfigLoader.h"
#include "Constants.h"

#include <QDir>
#include <QFileInfo>

MainConfig *PlasmaLogin::config()
{
    static MainConfig *s_instance = nullptr;
    if (s_instance) {
        return s_instance;
    }
    auto cfg = std::make_unique<KConfig>(QStringLiteral(CONFIG_FILE), KConfig::NoGlobals);
    QStringList sources;
    if (!QStringLiteral(SYSTEM_CONFIG_DIR).isEmpty()) {
        QDir sysDir(QStringLiteral(SYSTEM_CONFIG_DIR));
        if (sysDir.exists()) {
            const auto dirFiles = sysDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::LocaleAware);
            for (const QFileInfo &fi : dirFiles) {
                sources << fi.absoluteFilePath();
            }
        }
    }
    if (!QStringLiteral(CONFIG_DIR).isEmpty()) {
        QDir cfgDir(QStringLiteral(CONFIG_DIR));
        if (cfgDir.exists()) {
            const auto dirFiles = cfgDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::LocaleAware);
            for (const QFileInfo &fi : dirFiles) {
                sources << fi.absoluteFilePath();
            }
        }
    }
    cfg->addConfigSources(sources);
    s_instance = new MainConfig(std::move(cfg));
    return s_instance;
}
