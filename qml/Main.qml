import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: window

    width: 1380
    height: 860
    minimumWidth: 1120
    minimumHeight: 720
    visible: true
    title: "音乐播放器"
    flags: Qt.Window | Qt.FramelessWindowHint

    property bool lightMode: false
    property real discAngle: 0
    readonly property bool maximized: visibility === Window.Maximized
    readonly property int sceneMargin: maximized ? 14 : 28

    readonly property color accent: lightMode ? "#1b8fb2" : "#79f2cf"
    readonly property color accentSecondary: lightMode ? "#4d79f3" : "#55a6ff"
    readonly property color windowBase: lightMode ? "#edf3f8" : "#07111b"
    readonly property color panelBorder: lightMode ? "#22163143" : "#1dffffff"
    readonly property color textPrimary: lightMode ? "#12283b" : "#ffffff"
    readonly property color textSecondary: lightMode ? "#4f6a80" : "#96bfd4"
    readonly property color textMuted: lightMode ? "#617d92" : "#8eb0c0"
    readonly property color textDim: lightMode ? "#6e889b" : "#9cb8c6"
    readonly property color panelTintA: lightMode ? "#dbe9f5" : "#13273a"
    readonly property color panelTintB: lightMode ? "#eef4fa" : "#0d1522"
    readonly property color listItemColor: lightMode ? "#f8fbfe" : "#101a27"
    readonly property color listItemActiveColor: lightMode ? "#e2edf8" : "#1b2d40"
    readonly property color listBorderColor: lightMode ? "#1e163143" : "#12ffffff"
    readonly property color listBorderActiveColor: lightMode ? "#5d6ff0a8" : "#3d79f2cf"
    readonly property color badgeColor: lightMode ? "#d8e7f5" : "#173047"
    readonly property color badgeActiveColor: lightMode ? "#cae9f0" : "#2479f2cf"
    readonly property color discShellColor: lightMode ? "#f7fbff" : "#0b1520"
    readonly property color discShellBorder: lightMode ? "#31163143" : "#22ffffff"
    readonly property color discOverlay: lightMode ? "#08ffffff" : "#0a06111b"
    readonly property color errorColor: lightMode ? "#d05b4f" : "#ff9f8d"
    readonly property color titleBarLine: lightMode ? "#16163143" : "#12ffffff"

    color: windowBase

    background: Rectangle {
        color: window.windowBase

        Rectangle {
            width: parent.width * 0.62
            height: parent.height * 0.52
            x: -width * 0.14
            y: -height * 0.08
            radius: width / 2
            color: "#00000000"
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: window.lightMode ? "#4855a6ff" : "#3055a6ff"
                }
                GradientStop { position: 1.0; color: "#00000000" }
            }
            rotation: -18
        }

        Rectangle {
            width: parent.width * 0.46
            height: parent.height * 0.36
            x: parent.width - width * 0.85
            y: parent.height - height * 0.82
            radius: width / 2
            color: "#00000000"
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: window.lightMode ? "#3679f2cf" : "#2879f2cf"
                }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "#00000000"

            Canvas {
                anchors.fill: parent
                opacity: window.lightMode ? 0.10 : 0.18
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()
                    ctx.strokeStyle = window.lightMode ? "rgba(17,40,60,0.06)" : "rgba(255,255,255,0.05)"
                    ctx.lineWidth = 1

                    const gap = 54
                    for (let x = 0; x < width; x += gap) {
                        ctx.beginPath()
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, height)
                        ctx.stroke()
                    }

                    for (let y = 0; y < height; y += gap) {
                        ctx.beginPath()
                        ctx.moveTo(0, y)
                        ctx.lineTo(width, y)
                        ctx.stroke()
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: window.sceneMargin

        ColumnLayout {
            anchors.fill: parent
            spacing: 22

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 154
                lightMode: window.lightMode
                tintA: window.panelTintA
                tintB: window.panelTintB
                borderTint: window.panelBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onPressed: mouse => {
                                if (mouse.button === Qt.LeftButton) {
                                    window.startSystemMove()
                                }
                            }
                            onDoubleClicked: {
                                if (window.maximized) {
                                    window.showNormal()
                                } else {
                                    window.showMaximized()
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 14

                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                radius: 12
                                color: window.lightMode ? "#dce9f5" : "#163041"
                                border.width: 1
                                border.color: window.lightMode ? "#22163143" : "#18ffffff"

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: window.accent
                                }
                            }

                            ColumnLayout {
                                spacing: 1

                                Label {
                                    text: "GoodPlay 音乐播放器"
                                    color: window.textPrimary
                                    font.pixelSize: 16
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    text: "现代桌面播放器 · 环形频谱律动"
                                    color: window.textSecondary
                                    font.pixelSize: 11
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                spacing: 8

                                TitleBarButton {
                                    lightMode: window.lightMode
                                    buttonRole: "minimize"
                                    onClicked: window.showMinimized()
                                }

                                TitleBarButton {
                                    lightMode: window.lightMode
                                    buttonRole: "maximize"
                                    maximized: window.maximized
                                    onClicked: {
                                        if (window.maximized) {
                                            window.showNormal()
                                        } else {
                                            window.showMaximized()
                                        }
                                    }
                                }

                                TitleBarButton {
                                    lightMode: window.lightMode
                                    buttonRole: "close"
                                    danger: true
                                    onClicked: window.close()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        radius: 1
                        color: window.titleBarLine
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Repeater {
                            model: [
                                { label: "打开文件", width: 132, action: function() { playerController.openFiles() } },
                                { label: "打开文件夹", width: 144, action: function() { playerController.openFolder() } },
                                { label: window.lightMode ? "深色界面" : "浅色界面", width: 128, action: function() { window.lightMode = !window.lightMode } }
                            ]

                            delegate: ActionButton {
                                text: modelData.label
                                lightMode: window.lightMode
                                accentA: window.accentSecondary
                                accentB: window.accent
                                outlined: index < 2
                                highlighted: index === 2
                                radius: 16
                                implicitWidth: modelData.width
                                implicitHeight: 46
                                onClicked: modelData.action()
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 22

                GlassPanel {
                    Layout.preferredWidth: 360
                    Layout.fillHeight: true
                    lightMode: window.lightMode
                    tintA: window.panelTintA
                    tintB: window.panelTintB
                    borderTint: window.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 22
                        spacing: 18

                        Label {
                            text: "播放列表"
                            color: window.textPrimary
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        Label {
                            text: playerController.trackModel.count > 0
                                  ? playerController.trackModel.count + " 首歌曲"
                                  : "打开音频文件或文件夹后自动导入"
                            color: window.textMuted
                            font.pixelSize: 13
                        }

                        ListView {
                            id: playlistView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 10
                            rightMargin: 6
                            model: playerController.trackModel
                            currentIndex: playerController.currentIndex

                            delegate: Rectangle {
                                required property int index
                                required property string title
                                required property string subtitle
                                required property double duration
                                required property url coverArtUrl

                                width: playlistView.width - 6
                                height: 76
                                radius: 22
                                color: index === playerController.currentIndex
                                       ? window.listItemActiveColor
                                       : window.listItemColor
                                border.width: 1
                                border.color: index === playerController.currentIndex
                                              ? window.listBorderActiveColor
                                              : window.listBorderColor

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 14

                                    Item {
                                        Layout.preferredWidth: 50
                                        Layout.preferredHeight: 50

                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 18
                                            color: window.lightMode ? "#ffffff" : "#0d1520"
                                            border.width: 1
                                            border.color: index === playerController.currentIndex
                                                          ? window.listBorderActiveColor
                                                          : (window.lightMode ? "#22163143" : "#18ffffff")
                                        }

                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 3
                                            source: coverArtUrl
                                            fillMode: Image.PreserveAspectFit
                                            asynchronous: true
                                            cache: false
                                            mipmap: true
                                        }

                                        Rectangle {
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            anchors.rightMargin: -2
                                            anchors.bottomMargin: -2
                                            width: 16
                                            height: 16
                                            radius: 8
                                            visible: index === playerController.currentIndex
                                            color: window.accent
                                            border.width: 2
                                            border.color: window.lightMode ? "#ffffff" : "#0d1520"

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: 5
                                                height: 5
                                                radius: 2.5
                                                color: window.lightMode ? "#0f2b3e" : "#03111b"
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Label {
                                            text: title
                                            color: window.textPrimary
                                            font.pixelSize: 15
                                            font.weight: Font.Medium
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Label {
                                            text: subtitle
                                            color: window.textSecondary
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Label {
                                        text: duration > 0 ? playerController.formatTime(duration) : "--:--"
                                        color: window.textDim
                                        font.pixelSize: 12
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: playerController.playAt(index)
                                }
                            }

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AlwaysOff
                                visible: false
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 22

                    GlassPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        lightMode: window.lightMode
                        tintA: window.panelTintA
                        tintB: window.panelTintB
                        borderTint: window.panelBorder

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 28
                            spacing: 12

                            Label {
                                text: "正在播放"
                                color: window.textMuted
                                font.pixelSize: 14
                            }

                            Label {
                                text: playerController.currentTitle
                                color: window.textPrimary
                                font.pixelSize: 34
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: playerController.currentSubtitle
                                color: window.textSecondary
                                font.pixelSize: 15
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                SpectrumRing {
                                    id: visualizer
                                    anchors.centerIn: parent
                                    width: Math.min(parent.width, parent.height) * 0.84
                                    height: width
                                    values: playerController.spectrumValues
                                    accentColor: window.accent
                                    secondaryColor: window.accentSecondary
                                    active: playerController.playing
                                    lightMode: window.lightMode
                                }

                                Rectangle {
                                    id: centerDisc
                                    anchors.centerIn: parent
                                    width: Math.max(148, Math.min(276, visualizer.width * 0.34))
                                    height: width
                                    radius: width / 2
                                    color: window.discShellColor
                                    border.width: 2
                                    border.color: window.discShellBorder
                                    rotation: window.discAngle

                                    Image {
                                        anchors.fill: parent
                                        source: playerController.coverArtUrl
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                        cache: false
                                        mipmap: true
                                    }

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: width / 2
                                        color: window.discOverlay
                                    }

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: Math.max(22, centerDisc.width * 0.16)
                                        height: width
                                        radius: width / 2
                                        color: window.accent
                                        opacity: 0.92
                                        border.width: 2
                                        border.color: window.lightMode ? "#88ffffff" : "#ccffffff"
                                    }
                                }

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: centerDisc.width + Math.max(12, centerDisc.width * 0.08)
                                    height: width
                                    radius: width / 2
                                    color: "#00000000"
                                    border.width: 1
                                    border.color: window.lightMode ? "#18142c40" : "#18ffffff"
                                }

                                Timer {
                                    interval: 16
                                    repeat: true
                                    running: playerController.playing
                                    onTriggered: {
                                        window.discAngle = (window.discAngle + (360 * interval / 22000)) % 360
                                    }
                                }
                            }

                            Label {
                                visible: playerController.errorMessage.length > 0
                                text: playerController.errorMessage
                                color: window.errorColor
                                font.pixelSize: 13
                            }
                        }
                    }

                    GlassPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 176
                        lightMode: window.lightMode
                        tintA: window.panelTintA
                        tintB: window.panelTintB
                        borderTint: window.panelBorder

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 22
                            spacing: 16

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    text: playerController.formatTime(playerController.position)
                                    color: window.textPrimary
                                    font.pixelSize: 13
                                }

                                Item { Layout.fillWidth: true }

                                Label {
                                    text: playerController.formatTime(playerController.duration)
                                    color: window.textMuted
                                    font.pixelSize: 13
                                }
                            }

                            MediaSlider {
                                id: progressSlider
                                Layout.fillWidth: true
                                from: 0
                                to: Math.max(1, playerController.duration)
                                value: playerController.position
                                trackHeight: 8
                                handleSize: 20
                                trackColor: window.lightMode ? "#d8e7f2" : "#163145"
                                fillColor: window.accent
                                fillColorSecondary: window.accentSecondary
                                handleColor: window.lightMode ? "#ffffff" : "white"
                                handleBorderColor: window.accent
                                handleBorderWidth: 3

                                onMoved: playerController.setPosition(value)
                                onEditingFinished: playerController.setPosition(value)

                                Connections {
                                    target: playerController

                                    function onPositionChanged(position) {
                                        if (!progressSlider.pressed) {
                                            progressSlider.value = position
                                        }
                                    }

                                    function onDurationChanged(duration) {
                                        progressSlider.to = Math.max(1, duration)
                                        if (!progressSlider.pressed) {
                                            progressSlider.value = Math.min(playerController.position, progressSlider.to)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 14

                                Repeater {
                                    model: [
                                        { label: "上一首", width: 94, action: function() { playerController.previous() } },
                                        { label: playerController.playing ? "暂停" : "播放", width: 124, action: function() { playerController.playPause() } },
                                        { label: "下一首", width: 94, action: function() { playerController.next() } }
                                    ]

                                    delegate: ActionButton {
                                        text: modelData.label
                                        lightMode: window.lightMode
                                        accentA: window.accentSecondary
                                        accentB: window.accent
                                        highlighted: index === 1
                                        implicitWidth: modelData.width
                                        implicitHeight: 54
                                        radius: 18
                                        onClicked: modelData.action()
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                ColumnLayout {
                                    spacing: 6

                                    Label {
                                        text: "音量 " + Math.round(playerController.volume * 100) + "%"
                                        color: window.textPrimary
                                        font.pixelSize: 13
                                    }

                                    MediaSlider {
                                        id: volumeSlider
                                        Layout.preferredWidth: 220
                                        from: 0
                                        to: 1
                                        value: playerController.volume
                                        trackHeight: 6
                                        handleSize: 16
                                        trackColor: window.lightMode ? "#d8e7f2" : "#163145"
                                        fillColor: window.accent
                                        fillColorSecondary: window.accent
                                        handleColor: window.lightMode ? "#ffffff" : "white"
                                        handleBorderColor: window.accentSecondary
                                        handleBorderWidth: 2

                                        onMoved: playerController.setVolume(value)
                                        onEditingFinished: playerController.setVolume(value)

                                        Connections {
                                            target: playerController

                                            function onVolumeChanged() {
                                                if (!volumeSlider.pressed) {
                                                    volumeSlider.value = playerController.volume
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
