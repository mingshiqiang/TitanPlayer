import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import TitanPlayer.Native 1.0
import "qml"

Window {
    id: root

    width: 1100
    height: 680
    minimumWidth: 640
    minimumHeight: 400
    visible: true
    color: "#0a0a0c"
    title: player.currentFileName.length > 0 ? player.currentFileName : qsTr("TitanPlayer")
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinMaxButtonsHint

    property bool playlistVisible: true
    property bool fullscreen: visibility === Window.FullScreen
    property int resizeMargin: 5

    function toggleMaximized() {
        if (visibility === Window.Maximized)
            showNormal()
        else
            showMaximized()
    }

    function toggleFullscreen() {
        if (visibility === Window.FullScreen)
            showNormal()
        else
            showFullScreen()
    }

    function canSystemResize() {
        return visibility !== Window.Maximized && visibility !== Window.FullScreen
    }

    PlayerController {
        id: player
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Video Files")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Video Files (*.mp4 *.flv *.mkv *.avi *.mov *.webm *.ts *.wmv *.m4v)"),
            qsTr("All Files (*.*)")
        ]
        onAccepted: player.openUrls(selectedFiles)
    }

    SettingsDialog {
        id: settingsDialog
        player: player
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: openDialog.open()
    }

    Shortcut {
        sequence: "Space"
        onActivated: player.togglePlay()
    }

    Shortcut {
        sequence: "F"
        onActivated: root.toggleFullscreen()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: if (root.fullscreen) root.showNormal()
    }

    Shortcut {
        sequence: "L"
        onActivated: root.playlistVisible = !root.playlistVisible
    }

    Shortcut {
        sequence: "Left"
        onActivated: player.seek(player.position - 5000)
    }

    Shortcut {
        sequence: "Right"
        onActivated: player.seek(player.position + 5000)
    }

    Shortcut {
        sequence: "Up"
        onActivated: player.volume = Math.min(100, player.volume + 10)
    }

    Shortcut {
        sequence: "Down"
        onActivated: player.volume = Math.max(0, player.volume - 10)
    }

    Rectangle {
        anchors.fill: parent
        color: "#0a0a0c"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            visible: !root.fullscreen
            window: root
            fileName: player.currentFileName
            maximized: root.visibility === Window.Maximized
            playlistVisible: root.playlistVisible
            onOpenRequested: openDialog.open()
            onPlaylistToggleRequested: root.playlistVisible = !root.playlistVisible
            onMinimizeRequested: root.showMinimized()
            onMaximizeRequested: root.toggleMaximized()
            onCloseRequested: root.close()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            VideoArea {
                Layout.fillWidth: true
                Layout.fillHeight: true
                player: player
            }

            PlaylistPanel {
                Layout.fillHeight: true
                visible: root.playlistVisible && !root.fullscreen
                player: player
                onCloseRequested: root.playlistVisible = false
            }
        }

        ControlBar {
            Layout.fillWidth: true
            player: player
            onFullscreenRequested: root.toggleFullscreen()
            onSettingsRequested: settingsDialog.open()
        }
    }

    Rectangle {
        id: statusToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 92
        width: Math.min(statusText.implicitWidth + 32, parent.width - 80)
        height: 36
        radius: 18
        color: Qt.rgba(0.08, 0.08, 0.10, 0.90)
        border.color: Qt.rgba(1, 1, 1, 0.12)
        opacity: player.statusMessage.length > 0 ? 1 : 0

        Behavior on opacity { NumberAnimation { duration: 180 } }

        Text {
            id: statusText
            anchors.centerIn: parent
            width: parent.width - 24
            text: player.statusMessage
            color: "white"
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
        }
    }

    Timer {
        id: statusTimer
        interval: 3500
        running: player.statusMessage.length > 0
        repeat: false
        onTriggered: player.clearStatusMessage()
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.resizeMargin
        cursorShape: Qt.SizeHorCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.LeftEdge)
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.resizeMargin
        cursorShape: Qt.SizeHorCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.RightEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.resizeMargin
        cursorShape: Qt.SizeVerCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.TopEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.resizeMargin
        cursorShape: Qt.SizeVerCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.BottomEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        width: root.resizeMargin * 2
        height: root.resizeMargin * 2
        cursorShape: Qt.SizeFDiagCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        width: root.resizeMargin * 2
        height: root.resizeMargin * 2
        cursorShape: Qt.SizeBDiagCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }

    MouseArea {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: root.resizeMargin * 2
        height: root.resizeMargin * 2
        cursorShape: Qt.SizeBDiagCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }

    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: root.resizeMargin * 2
        height: root.resizeMargin * 2
        cursorShape: Qt.SizeFDiagCursor
        enabled: root.canSystemResize()
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }
}
