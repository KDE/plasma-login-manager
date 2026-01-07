import QtQuick
import QtQuick.Window
import org.kde.breeze.components as BreezeComponents

Item {
	id: main
	anchors.fill: parent

	property alias wallpaperContainer: wallpaperPlaceholder

	Item {
		id: wallpaperPlaceholder
		anchors.fill: parent
	}

	BreezeComponents.WallpaperFader {
		anchors.fill: parent
		state: Window.window.blur ? "on" : "off"
		source: wallpaperPlaceholder.children[0]
	}
}
