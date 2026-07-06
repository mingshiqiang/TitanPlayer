import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: root

    property var player
    property url selectedFolderUrl: ""
    property string folderText: player ? player.screenshotDir : ""

    title: qsTr("Settings")
    modal: true
    width: 640
    height: 420
    padding: 0
    closePolicy: Popup.CloseOnEscape

    onOpened: {
        selectedFolderUrl = ""
        folderText = player ? player.screenshotDir : ""
        formatCombo.currentIndex = player && player.screenshotFormat === "jpg" ? 1 : 0
    }

    background: Rectangle {
        radius: 8
        color: "#0a0a0c"
        border.color: Qt.rgba(1, 1, 1, 0.12)
    }

    header: Rectangle {
        height: 40
        color: "#141418"
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 8

            Text {
                text: qsTr("Settings")
                color: "white"
                font.pixelSize: 14
                font.weight: Font.Medium
            }

            Item { Layout.fillWidth: true }

            IconButton {
                buttonSize: 28
                iconExtent: 14
                iconSource: "qrc:/icons/resources/close.svg"
                onClicked: root.reject()
            }
        }
    }

    contentItem: RowLayout {
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 160
            Layout.fillHeight: true
            color: "#0e0e12"

            Text {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 18
                anchors.leftMargin: 16
                text: qsTr("Screenshot")
                color: "white"
                font.pixelSize: 13
            }

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: 12
                width: 3
                height: 40
                color: "#7c3aed"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0a0a0c"

            GridLayout {
                anchors.fill: parent
                anchors.margins: 28
                columns: 3
                columnSpacing: 12
                rowSpacing: 18

                Text {
                    text: qsTr("Screenshot folder:")
                    color: Qt.rgba(1, 1, 1, 0.85)
                    font.pixelSize: 13
                }

                TextField {
                    Layout.fillWidth: true
                    text: root.folderText
                    readOnly: true
                    color: "white"
                    selectedTextColor: "white"
                    selectionColor: "#7c3aed"
                    background: Rectangle {
                        radius: 6
                        color: "#1a1a20"
                        border.color: Qt.rgba(1, 1, 1, 0.12)
                    }
                }

                Button {
                    text: qsTr("Browse...")
                    focusPolicy: Qt.NoFocus
                    onClicked: folderDialog.open()
                }

                Text {
                    text: qsTr("Image format:")
                    color: Qt.rgba(1, 1, 1, 0.85)
                    font.pixelSize: 13
                }

                ComboBox {
                    id: formatCombo
                    Layout.preferredWidth: 120
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { text: qsTr("PNG"), value: "png" },
                        { text: qsTr("JPG"), value: "jpg" }
                    ]
                }

                Item { Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
            }
        }
    }

    footer: Rectangle {
        height: 56
        color: "#0e0e12"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 10

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancel")
                focusPolicy: Qt.NoFocus
                onClicked: root.reject()
            }

            Button {
                text: qsTr("OK")
                focusPolicy: Qt.NoFocus
                highlighted: true
                onClicked: {
                    if (root.player)
                        root.player.setScreenshotSettings(root.selectedFolderUrl, formatCombo.currentValue)
                    root.accept()
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Select screenshot folder")
        onAccepted: {
            root.selectedFolderUrl = selectedFolder
            root.folderText = selectedFolder.toString().replace("file:///", "")
        }
    }
}
