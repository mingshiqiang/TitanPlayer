import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var window
    property string fileName: ""
    property bool maximized: false
    property bool playlistVisible: true

    signal openRequested()
    signal playlistToggleRequested()
    signal minimizeRequested()
    signal maximizeRequested()
    signal closeRequested()

    height: 40
    color: "transparent"

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.36) }
        GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.0) }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: {
            if (root.window)
                root.window.startSystemMove()
        }
        onDoubleClicked: root.maximizeRequested()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        Image {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            source: "qrc:/icons/resources/logo.svg"
            sourceSize.width: 24
            sourceSize.height: 24
            fillMode: Image.PreserveAspectFit
        }

        Text {
            Layout.fillWidth: true
            text: root.fileName.length > 0 ? root.fileName : qsTr("TitanPlayer")
            color: "white"
            elide: Text.ElideMiddle
            font.pixelSize: 13
            font.weight: Font.Medium
            verticalAlignment: Text.AlignVCenter
        }

        IconButton {
            buttonSize: 28
            iconExtent: 16
            iconSource: "qrc:/icons/resources/load_video.svg"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Load video")
            onClicked: root.openRequested()
        }

        IconButton {
            buttonSize: 28
            iconExtent: 16
            iconSource: "qrc:/icons/resources/expand_list.svg"
            opacity: root.playlistVisible ? 1.0 : 0.55
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Toggle playlist")
            onClicked: root.playlistToggleRequested()
        }

        IconButton {
            buttonSize: 28
            iconExtent: 16
            iconSource: "qrc:/icons/resources/minimize.svg"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Minimize")
            onClicked: root.minimizeRequested()
        }

        IconButton {
            buttonSize: 28
            iconExtent: 16
            iconSource: root.maximized ? "qrc:/icons/resources/normal.svg" : "qrc:/icons/resources/max.svg"
            ToolTip.visible: hovered
            ToolTip.text: root.maximized ? qsTr("Restore") : qsTr("Maximize")
            onClicked: root.maximizeRequested()
        }

        IconButton {
            buttonSize: 28
            iconExtent: 16
            iconSource: "qrc:/icons/resources/close.svg"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Close")
            onClicked: root.closeRequested()
        }
    }
}
