import QtQuick

Item {
    id: root

    property bool lightMode: false
    property bool danger: false
    property bool maximized: false
    property string buttonRole: "minimize"

    signal clicked()

    implicitWidth: 42
    implicitHeight: 32

    function alpha(colorValue, opacity) {
        return Qt.rgba(colorValue.r, colorValue.g, colorValue.b, opacity)
    }

    readonly property color baseColor: lightMode ? Qt.rgba(1, 1, 1, danger ? 0.78 : 0.72)
                                                 : Qt.rgba(1, 1, 1, 0.06)
    readonly property color hoverColor: danger
                                        ? (lightMode ? "#ffdedd" : "#6a2422")
                                        : (lightMode ? "#dfeaf3" : "#162635")
    readonly property color pressedColor: danger
                                          ? (lightMode ? "#f4c9c6" : "#7c2a28")
                                          : (lightMode ? "#d0deea" : "#1d3142")
    readonly property color borderColor: danger
                                         ? alpha(Qt.rgba(0.84, 0.28, 0.22, 1), lightMode ? 0.34 : 0.44)
                                         : (lightMode ? Qt.rgba(0.08, 0.18, 0.26, 0.16)
                                                      : Qt.rgba(1, 1, 1, 0.10))
    readonly property color iconColor: danger
                                       ? (lightMode ? "#8f241e" : "#ffe9e8")
                                       : (lightMode ? "#10283b" : "#eef8ff")

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: area.pressed ? root.pressedColor : area.containsMouse ? root.hoverColor : root.baseColor
        border.width: 1
        border.color: root.borderColor
    }

    Canvas {
        id: iconCanvas
        anchors.fill: parent
        anchors.margins: 9
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = root.iconColor
            ctx.fillStyle = root.iconColor
            ctx.lineWidth = 1.8
            ctx.lineCap = "round"

            if (root.buttonRole === "minimize") {
                ctx.beginPath()
                ctx.moveTo(width * 0.20, height * 0.72)
                ctx.lineTo(width * 0.80, height * 0.72)
                ctx.stroke()
            } else if (root.buttonRole === "maximize") {
                if (root.maximized) {
                    ctx.strokeRect(width * 0.30, height * 0.22, width * 0.42, height * 0.42)
                    ctx.strokeRect(width * 0.20, height * 0.34, width * 0.42, height * 0.42)
                } else {
                    ctx.strokeRect(width * 0.24, height * 0.24, width * 0.52, height * 0.52)
                }
            } else {
                ctx.beginPath()
                ctx.moveTo(width * 0.26, height * 0.26)
                ctx.lineTo(width * 0.74, height * 0.74)
                ctx.moveTo(width * 0.74, height * 0.26)
                ctx.lineTo(width * 0.26, height * 0.74)
                ctx.stroke()
            }
        }
    }

    onLightModeChanged: iconCanvas.requestPaint()
    onDangerChanged: iconCanvas.requestPaint()
    onMaximizedChanged: iconCanvas.requestPaint()
    onButtonRoleChanged: iconCanvas.requestPaint()
    onIconColorChanged: iconCanvas.requestPaint()
    Component.onCompleted: iconCanvas.requestPaint()

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onContainsMouseChanged: iconCanvas.requestPaint()
        onPressedChanged: iconCanvas.requestPaint()
        onClicked: root.clicked()
    }
}
