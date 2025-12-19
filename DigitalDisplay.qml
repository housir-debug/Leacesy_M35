import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Rectangle {
    id: display
    width: 200
    height: 80
    radius: 10
    color: "#0f3460"
    border.width: 2
    border.color: "#00b4d8"

    property string label: "电流"
    property string unit: "A"
    property real value: 0
    property int precision: 3
    property color textColor: "#ffffff"

    layer.enabled: true
    layer.effect: DropShadow {
        transparentBorder: true
        horizontalOffset: 3
        verticalOffset: 3
        radius: 8
        samples: 17
        color: "#20000000"
    }

    Column {
        anchors.fill: parent
        anchors.margins: 10

        Text {
            text: label
            color: "#cccccc"
            font.pixelSize: 14
        }

        Row {
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                text: value.toFixed(precision)
                color: textColor
                font.pixelSize: 28
                font.bold: true
                font.family: "Digital-7"
            }

            Text {
                text: unit
                color: "#00b4d8"
                font.pixelSize: 18
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 5
            }
        }
    }
}
