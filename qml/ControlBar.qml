import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var player
    signal fullscreenRequested()
    signal settingsRequested()

    height: 74
    color: Qt.rgba(0.08, 0.08, 0.10, 0.88)

    function formatTime(ms) {
        var total = Math.max(0, Math.floor(ms / 1000))
        var h = Math.floor(total / 3600)
        var m = Math.floor((total % 3600) / 60)
        var s = total % 60
        function pad(v) { return v < 10 ? "0" + v : "" + v }
        return h > 0 ? h + ":" + pad(m) + ":" + pad(s) : pad(m) + ":" + pad(s)
    }

    function speedLabel(value) {
        return Math.abs(value - Math.round(value)) < 0.001 ? Math.round(value) + "x" : value + "x"
    }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Qt.rgba(1, 1, 1, 0.06)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.topMargin: 6
        anchors.bottomMargin: 8
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                Layout.preferredWidth: 52
                text: root.formatTime(seekSlider.pressed ? seekSlider.value : (root.player ? root.player.position : 0))
                color: Qt.rgba(1, 1, 1, 0.70)
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
                font.family: "Consolas"
            }

            Slider {
                id: seekSlider
                Layout.fillWidth: true
                from: 0
                to: root.player && root.player.duration > 0 ? root.player.duration : 1
                value: 0
                enabled: root.player && root.player.duration > 0
                live: false
                onMoved: {
                    if (root.player)
                        root.player.seek(Math.round(value))
                }

                Binding on value {
                    when: !seekSlider.pressed
                    value: root.player ? root.player.position : 0
                }
            }

            Text {
                Layout.preferredWidth: 52
                text: root.formatTime(root.player ? root.player.duration : 0)
                color: Qt.rgba(1, 1, 1, 0.70)
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
                font.family: "Consolas"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                buttonSize: 44
                iconExtent: 22
                iconSource: root.player && root.player.playing ? "qrc:/icons/resources/pause.svg" : "qrc:/icons/resources/play.svg"
                ToolTip.visible: hovered
                ToolTip.text: root.player && root.player.playing ? qsTr("Pause") : qsTr("Play")
                onClicked: if (root.player) root.player.togglePlay()
            }

            IconButton {
                iconSource: "qrc:/icons/resources/previous.svg"
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Previous")
                onClicked: if (root.player) root.player.previous()
            }

            IconButton {
                iconSource: "qrc:/icons/resources/next.svg"
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Next")
                onClicked: if (root.player) root.player.next()
            }

            Item { Layout.preferredWidth: 4 }

            IconButton {
                id: volumeButton
                iconSource: root.player && root.player.volume === 0 ? "qrc:/icons/resources/mute.svg" : "qrc:/icons/resources/volume.svg"
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Volume")
                onClicked: volumePopup.openAbove(volumeButton)
            }

            Item { Layout.fillWidth: true }

            Button {
                id: speedButton
                Layout.preferredWidth: 52
                Layout.preferredHeight: 32
                text: root.speedLabel(root.player ? root.player.speed : 1.0)
                focusPolicy: Qt.NoFocus
                onClicked: speedPopup.openAbove(speedButton)
                background: Rectangle {
                    radius: 16
                    color: Math.abs((root.player ? root.player.speed : 1.0) - 1.0) < 0.001
                           ? "transparent" : Qt.rgba(0.49, 0.23, 0.93, 0.25)
                    border.color: Math.abs((root.player ? root.player.speed : 1.0) - 1.0) < 0.001
                                  ? Qt.rgba(1, 1, 1, 0.20) : Qt.rgba(0.49, 0.23, 0.93, 0.60)
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.family: "Consolas"
                }
            }

            IconButton {
                iconSource: "qrc:/icons/resources/screenshot.svg"
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Screenshot")
                onClicked: if (root.player) root.player.takeScreenshot()
            }

            IconButton {
                iconSource: "qrc:/icons/resources/full_screen.svg"
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Fullscreen")
                onClicked: root.fullscreenRequested()
            }

            IconButton {
                iconSource: "qrc:/icons/resources/set.svg"
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Settings")
                onClicked: root.settingsRequested()
            }
        }
    }

    VolumePopup {
        id: volumePopup
        parent: root
        volume: root.player ? root.player.volume : 100
        onVolumeSelected: function(value) {
            if (root.player)
                root.player.volume = value
        }
    }

    SpeedPopup {
        id: speedPopup
        parent: root
        speed: root.player ? root.player.speed : 1.0
        onSpeedSelected: function(value) {
            if (root.player)
                root.player.speed = value
        }
    }
}
