import QtQuick

Item {
    id: root

    property real from: 0
    property real to: 1
    property real value: 0
    property bool pressed: dragArea.pressed
    property real trackHeight: 8
    property real handleSize: 20
    property color trackColor: "#163145"
    property color fillColor: "#79f2cf"
    property color fillColorSecondary: fillColor
    property color handleColor: "white"
    property color handleBorderColor: "#79f2cf"
    property real handleBorderWidth: 3

    signal moved(real value)
    signal editingFinished(real value)

    implicitWidth: 240
    implicitHeight: Math.max(handleSize, trackHeight) + 8

    function clamp(v, minV, maxV) {
        return Math.max(minV, Math.min(maxV, v))
    }

    function normalizedValue() {
        const span = to - from
        if (span <= 0)
            return 0
        return clamp((value - from) / span, 0, 1)
    }

    function updateFromMouse(mouseX) {
        const usableWidth = Math.max(1, width - handleSize)
        const localX = clamp(mouseX - handleSize / 2, 0, usableWidth)
        const ratio = localX / usableWidth
        const nextValue = from + (to - from) * ratio
        value = nextValue
        moved(nextValue)
    }

    Rectangle {
        id: track
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: root.trackHeight
        radius: height / 2
        color: root.trackColor

        Rectangle {
            width: root.normalizedValue() * parent.width
            height: parent.height
            radius: parent.radius

            gradient: Gradient {
                GradientStop { position: 0.0; color: root.fillColorSecondary }
                GradientStop { position: 1.0; color: root.fillColor }
            }
        }
    }

    Rectangle {
        id: handle
        width: root.handleSize
        height: root.handleSize
        radius: width / 2
        color: root.handleColor
        border.width: root.handleBorderWidth
        border.color: root.handleBorderColor
        x: root.normalizedValue() * (root.width - width)
        y: (root.height - height) / 2
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onPressed: mouse => {
            root.updateFromMouse(mouse.x)
        }

        onPositionChanged: mouse => {
            if (pressed) {
                root.updateFromMouse(mouse.x)
            }
        }

        onReleased: mouse => {
            root.updateFromMouse(mouse.x)
            root.editingFinished(root.value)
        }
    }
}
