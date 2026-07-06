import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var player
    signal closeRequested()

    width: 300
    color: Qt.rgba(0.08, 0.08, 0.10, 0.90)

    function formatTime(ms) {
        if (!ms || ms <= 0)
            return ""
        var total = Math.floor(ms / 1000)
        var h = Math.floor(total / 3600)
        var m = Math.floor((total % 3600) / 60)
        var s = total % 60
        function pad(v) { return v < 10 ? "0" + v : "" + v }
        return h > 0 ? h + ":" + pad(m) + ":" + pad(s) : pad(m) + ":" + pad(s)
    }

    Rectangle {
        anchors.left: parent.left
        width: 1
        height: parent.height
        color: Qt.rgba(1, 1, 1, 0.06)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.leftMargin: 16
            Layout.rightMargin: 12
            spacing: 8

            Text {
                text: qsTr("Playlist")
                color: "white"
                font.pixelSize: 14
                font.weight: Font.Medium
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("%1 items").arg(root.player ? root.player.playlist.length : 0)
                color: Qt.rgba(1, 1, 1, 0.45)
                font.pixelSize: 12
            }

            Button {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                text: ">"
                focusPolicy: Qt.NoFocus
                onClicked: root.closeRequested()
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.10) : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: Qt.rgba(1, 1, 1, 0.75)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.player ? root.player.playlist : []

            delegate: Rectangle {
                width: ListView.view.width
                height: 56
                color: modelData.current ? Qt.rgba(0.49, 0.23, 0.93, 0.35)
                                         : (mouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent")

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 4

                    Text {
                        width: parent.width
                        text: modelData.fileName
                        color: "white"
                        elide: Text.ElideMiddle
                        font.pixelSize: 13
                        font.bold: modelData.current
                    }

                    Text {
                        width: parent.width
                        text: root.formatTime(modelData.duration)
                        visible: text.length > 0
                        color: Qt.rgba(1, 1, 1, 0.45)
                        font.pixelSize: 11
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.04)
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    onDoubleClicked: if (root.player) root.player.playAt(modelData.index)
                }
            }
        }
    }
}
