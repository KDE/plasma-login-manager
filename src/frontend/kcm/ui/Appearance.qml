/*
SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>

SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

Kirigami.Page {
    // The following two must be set for correct alignment with wallpaper config
    id: appearanceRoot
    property alias parentLayout: parentLayout
    // Plugins expect these two properties
    property var wallpaper: kcm.wallpaperIntegration
    property var configDialog: kcm

    title: i18nc("@title", "Appearance")

    padding: 6  // Layout_ChildMarginWidth from Breeze

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        Kirigami.FormLayout {
            id: parentLayout // Don't change needed for correct alignment with wallpaper config

            QQC2.CheckBox {
                Kirigami.FormData.label: i18n("Clock")
                text: i18n("Show clock")

                checked: kcm.settings.showClock
                onToggled: kcm.settings.showClock = checked

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "ShowClock"
                }
            }

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Wallpaper type:")
                model: kcm.availableWallpaperPlugins()
                textRole: "name"
                valueRole: "id"
                currentIndex: model.findIndex(wallpaper => wallpaper["id"] === kcm.settings.wallpaperPluginId)
                displayText: model[currentIndex]["name"]

                onActivated: {
                    kcm.settings.wallpaperPluginId = model[index]["id"]
                }

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "WallpaperPluginId"
                }
            }
        }

        WallpaperConfig {
            sourceFile: kcm.wallpaperConfigFile
            onConfigurationChanged: kcm.updateState()
            onConfigurationForceChanged: kcm.forceUpdateState()
            // Cancel out page margins so it touches the edges
            Layout.margins: -appearanceRoot.padding
        }
    }
}
