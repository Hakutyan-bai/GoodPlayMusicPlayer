import QtQuick

Item {
    id: root

    property var values: []
    property bool lightMode: false
    property color accentColor: "#79f2cf"
    property color secondaryColor: "#55a6ff"
    property bool active: false

    function mixedColor(t, alpha) {
        const r = accentColor.r + (secondaryColor.r - accentColor.r) * t
        const g = accentColor.g + (secondaryColor.g - accentColor.g) * t
        const b = accentColor.b + (secondaryColor.b - accentColor.b) * t
        return Qt.rgba(r, g, b, alpha)
    }

    onValuesChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    onActiveChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const w = width
            const h = height
            const cx = w / 2
            const cy = h / 2
            const half = Math.min(w, h) / 2
            const discRadius = half * 0.33
            const innerRadius = discRadius + 26
            const maxBarLength = Math.max(32, half - innerRadius - 16)
            const count = Math.max(1, root.values.length)
            const glow = root.active ? 1.0 : 0.55

            ctx.beginPath()
            const halo = ctx.createRadialGradient(cx, cy, discRadius * 0.5, cx, cy, half - 6)
            halo.addColorStop(0.0, root.lightMode ? Qt.rgba(0.19, 0.58, 0.73, 0.14 * glow)
                                                  : Qt.rgba(0.48, 0.95, 0.82, 0.10 * glow))
            halo.addColorStop(0.62, root.lightMode ? Qt.rgba(0.31, 0.53, 1.0, 0.10 * glow)
                                                   : Qt.rgba(0.33, 0.65, 1.0, 0.08 * glow))
            halo.addColorStop(1.0, Qt.rgba(0.0, 0.0, 0.0, 0.0))
            ctx.fillStyle = halo
            ctx.arc(cx, cy, half - 8, 0, Math.PI * 2)
            ctx.fill()

            for (let i = 0; i < count; ++i) {
                const value = Number(root.values[i] || 0)
                const angle = (Math.PI * 2 * i) / count - Math.PI / 2
                const strength = Math.min(1.0, Math.max(0.0, value))
                const outerRadius = innerRadius + maxBarLength * (0.30 + strength * 0.70)
                const startX = cx + Math.cos(angle) * innerRadius
                const startY = cy + Math.sin(angle) * innerRadius
                const endX = cx + Math.cos(angle) * outerRadius
                const endY = cy + Math.sin(angle) * outerRadius
                const widthScale = 2.4 + strength * 4.4

                ctx.beginPath()
                ctx.moveTo(startX, startY)
                ctx.lineTo(endX, endY)
                ctx.lineWidth = widthScale
                ctx.lineCap = "round"
                ctx.strokeStyle = root.mixedColor(i / count, ((root.lightMode ? 0.44 : 0.34) + strength * 0.66) * glow)
                ctx.stroke()
            }

            ctx.beginPath()
            ctx.strokeStyle = root.lightMode ? Qt.rgba(0.08, 0.18, 0.26, 0.18) : Qt.rgba(1, 1, 1, 0.10)
            ctx.lineWidth = 1.5
            ctx.arc(cx, cy, innerRadius - 14, 0, Math.PI * 2)
            ctx.stroke()

            ctx.beginPath()
            ctx.strokeStyle = root.lightMode ? Qt.rgba(0.08, 0.18, 0.26, 0.08) : Qt.rgba(1, 1, 1, 0.06)
            ctx.lineWidth = 1.0
            ctx.arc(cx, cy, innerRadius - 26, 0, Math.PI * 2)
            ctx.stroke()
        }
    }
}
