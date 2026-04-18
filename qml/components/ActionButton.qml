import QtQuick

Item {
    id: root

    property alias text: label.text
    property bool lightMode: false
    property bool highlighted: false
    property bool outlined: false
    property color accentA: "#79f2cf"
    property color accentB: "#55a6ff"
    property int radius: 18

    signal clicked()

    implicitWidth: 132
    implicitHeight: 48

    function mixColor(a, b, t, alpha) {
        return Qt.rgba(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            alpha === undefined ? (a.a + (b.a - a.a) * t) : alpha
        )
    }

    function alpha(colorValue, opacity) {
        return Qt.rgba(colorValue.r, colorValue.g, colorValue.b, opacity)
    }

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: root.radius
        border.width: 1
        border.color: root.lightMode
                      ? (root.highlighted
                         ? root.alpha(root.mixColor(root.accentA, root.accentB, 0.32), area.pressed ? 0.74 : 0.54)
                         : root.outlined
                           ? root.alpha(root.mixColor(root.accentA, root.accentB, 0.45), area.pressed ? 0.42 : 0.26)
                           : Qt.rgba(0.08, 0.18, 0.26, area.pressed ? 0.18 : 0.10))
                      : (root.highlighted
                         ? root.alpha(root.mixColor(root.accentA, root.accentB, 0.55), area.pressed ? 0.78 : 0.52)
                         : root.outlined
                           ? root.alpha(root.accentA, area.pressed ? 0.58 : 0.34)
                           : Qt.rgba(1, 1, 1, area.pressed ? 0.20 : 0.12))

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: root.lightMode
                       ? (root.highlighted
                          ? (area.pressed
                             ? root.mixColor(root.accentA, root.accentB, 0.20, 0.96)
                             : area.containsMouse
                               ? root.mixColor(root.accentA, root.accentB, 0.16, 0.92)
                               : root.mixColor(root.accentA, root.accentB, 0.12, 0.88))
                          : (area.pressed ? "#d5e3ee" : area.containsMouse ? "#eef5fb" : "#f9fbff"))
                       : (root.highlighted
                          ? (area.pressed
                             ? root.mixColor(root.accentB, root.accentA, 0.24, 0.42)
                             : area.containsMouse
                               ? root.mixColor(root.accentB, root.accentA, 0.20, 0.36)
                               : root.mixColor(root.accentB, root.accentA, 0.16, 0.30))
                          : (area.pressed ? "#162737" : area.containsMouse ? "#132333" : "#101c2a"))
            }
            GradientStop {
                position: 1.0
                color: root.lightMode
                       ? (root.highlighted
                          ? (area.pressed
                             ? root.mixColor(root.accentA, root.accentB, 0.72, 0.94)
                             : area.containsMouse
                               ? root.mixColor(root.accentA, root.accentB, 0.66, 0.90)
                               : root.mixColor(root.accentA, root.accentB, 0.60, 0.86))
                          : (area.pressed ? "#c8d9e5" : area.containsMouse ? "#e1ecf4" : "#eef4f9"))
                       : (root.highlighted
                          ? (area.pressed
                             ? root.mixColor(root.accentA, root.accentB, 0.75, 0.30)
                             : area.containsMouse
                               ? root.mixColor(root.accentA, root.accentB, 0.72, 0.24)
                               : root.mixColor(root.accentA, root.accentB, 0.68, 0.20))
                          : (area.pressed ? "#0d1823" : area.containsMouse ? "#102030" : "#0c1620"))
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, root.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.lightMode
                      ? Qt.rgba(1, 1, 1, area.containsMouse ? 0.22 : 0.12)
                      : Qt.rgba(1, 1, 1, area.containsMouse ? 0.10 : 0.05)
    }

    Text {
        id: label
        anchors.centerIn: parent
        color: root.lightMode
               ? (root.highlighted ? "#10334d" : "#183247")
               : (root.highlighted ? "#f4fffd" : "#eef8ff")
        font.pixelSize: 14
        font.weight: Font.Medium
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
