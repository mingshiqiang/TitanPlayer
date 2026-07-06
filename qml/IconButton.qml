import QtQuick
import QtQuick.Controls

Button {
    id: root

    property url iconSource
    property int buttonSize: 38
    property int iconExtent: 18

    width: buttonSize
    height: buttonSize
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: width / 2
        color: root.hovered ? Qt.rgba(1, 1, 1, 0.10) : "transparent"
    }

    contentItem: Image {
        source: root.iconSource
        width: root.iconExtent
        height: root.iconExtent
        sourceSize.width: root.iconExtent
        sourceSize.height: root.iconExtent
        fillMode: Image.PreserveAspectFit
        anchors.centerIn: parent
    }
}
