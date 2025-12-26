import QtQuick 2.15
import QtQuick.Shapes 1.15

Rectangle {
    id: waveform
    width: 400
    height: 200
    color: "#1a1a2e"
    radius: 8
    border.color: "#000000"

    property var dataPoints: []
    property color waveColor: "#00b4d8"
    property color gridColor: "#16213e"

    // 网格背景
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.strokeStyle = gridColor
            ctx.lineWidth = 1

            // 水平线
            for (var i = 1; i < 5; i++) {
                ctx.beginPath()
                ctx.moveTo(0, i * height / 5)
                ctx.lineTo(width, i * height / 5)
                ctx.stroke()
            }

            // 垂直线
            for (var j = 1; j < 10; j++) {
                ctx.beginPath()
                ctx.moveTo(j * width / 10, 0)
                ctx.lineTo(j * width / 10, height)
                ctx.stroke()
            }
        }
    }

    // 波形路径
    Shape {
        anchors.fill: parent
        ShapePath {
            id: wavePath
            strokeColor: waveColor
            strokeWidth: 2
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            PathSvg {
                path: generateWavePath()
            }
        }
    }

    function generateWavePath() {
        if (dataPoints.length < 2) return ""

        var path = "M 0 " + (height / 2)
        var stepX = width / (dataPoints.length - 1)

        for (var i = 1; i < dataPoints.length; i++) {
            var x = i * stepX
            var y = height / 2 - dataPoints[i] * (height / 2)
            path += " L " + x + " " + y
        }

        return path
    }
}
