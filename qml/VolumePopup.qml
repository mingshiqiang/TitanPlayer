import QtQuick
import QtQuick.Controls

Popup {
    id: root

    property int volume: 100
    signal volumeSelected(int value)

    width: 44
    height: 150
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    function openAbove(button) {
        x = button.x + button.width / 2 - width / 2
        y = button.y - height - 6
        open()
    }

    background: Rectangle {
        radius: 8
        color: Qt.rgba(0.08, 0.08, 0.10, 0.94)
        border.color: Qt.rgba(1, 1, 1, 0.12)
    }

    contentItem: Item {
        Slider {
            id: slider
            anchors.centerIn: parent
            width: 28
            height: 120
            from: 0
            to: 100
            orientation: Qt.Vertical
            value: root.volume
            live: true
            onMoved: root.volumeSelected(Math.round(value))
        }
    }
}
