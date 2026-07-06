import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    property real speed: 1.0
    readonly property var presets: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0]
    signal speedSelected(real value)

    width: 104
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 4

    function labelFor(value) {
        return Math.abs(value - Math.round(value)) < 0.001 ? Math.round(value) + "x" : value + "x"
    }

    function openAbove(button) {
        x = button.x
        y = button.y - height - 4
        open()
    }

    background: Rectangle {
        radius: 8
        color: Qt.rgba(0.08, 0.08, 0.10, 0.96)
        border.color: Qt.rgba(1, 1, 1, 0.12)
    }

    contentItem: ColumnLayout {
        spacing: 2

        Repeater {
            model: root.presets

            delegate: Button {
                Layout.fillWidth: true
                height: 28
                text: root.labelFor(modelData)
                focusPolicy: Qt.NoFocus
                highlighted: Math.abs(root.speed - modelData) < 0.001
                onClicked: {
                    root.speedSelected(modelData)
                    root.close()
                }
                background: Rectangle {
                    radius: 4
                    color: parent.highlighted ? Qt.rgba(0.49, 0.23, 0.93, 0.42)
                                              : (parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    font.family: "Consolas"
                }
            }
        }
    }
}
