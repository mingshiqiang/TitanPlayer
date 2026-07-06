import QtQuick
import QtQuick.Controls
import TitanPlayer.Native 1.0

Item {
    id: root

    property var player

    Rectangle {
        anchors.fill: parent
        color: "#0a0a0c"
    }

    VideoOutputItem {
        anchors.fill: parent
        frameSource: root.player ? root.player.videoFrameSource : null
    }

    Label {
        anchors.centerIn: parent
        visible: !root.player || root.player.currentFileName.length === 0
        text: qsTr("Drop a video file here, or click load video\n\nSupported: MP4 / FLV / MKV / AVI / MOV / WEBM / TS")
        horizontalAlignment: Text.AlignHCenter
        color: Qt.rgba(1, 1, 1, 0.42)
        font.pixelSize: 15
        font.weight: Font.Medium
    }

    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            if (drop.hasUrls && root.player) {
                root.player.openUrls(drop.urls)
                drop.acceptProposedAction()
            }
        }
    }
}
