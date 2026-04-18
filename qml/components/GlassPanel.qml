import QtQuick

Rectangle {
    id: root

    property bool lightMode: false
    property color tintA: "#13273a"
    property color tintB: "#0d1522"
    property color borderTint: "#26ffffff"

    radius: 28
    color: lightMode ? Qt.rgba(1, 1, 1, 0.78) : "transparent"
    border.width: 1
    border.color: borderTint

    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: lightMode ? Qt.rgba(0.98, 0.99, 1.0, 0.98) : Qt.rgba(0.10, 0.17, 0.25, 0.92)
        }
        GradientStop {
            position: 1.0
            color: lightMode ? Qt.rgba(0.91, 0.95, 0.99, 0.94) : Qt.rgba(0.05, 0.08, 0.13, 0.88)
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: parent.radius - 1
        color: "transparent"
        border.width: 1
        border.color: lightMode ? "#26ffffff" : "#08ffffff"
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.alpha(root.tintA, lightMode ? 0.16 : 0.28)
            }
            GradientStop {
                position: 1.0
                color: Qt.alpha(root.tintB, lightMode ? 0.12 : 0.16)
            }
        }
    }
}
