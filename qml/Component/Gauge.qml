import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Rectangle {
    id: gaugeRoot
    width: 300
    height: 300
    radius: width / 2
    color: "#1a1a2e"

    property string title: "电压"
    property string unit: "V"
    property real value: 0
    property real minValue: 0
    property real maxValue: 100
    property color gaugeColor: "#00b4d8"
    property bool showDigital: true

    // 外环光泽效果
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 2
        border.color: "#16213e"

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 20
            height: parent.height - 20
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(1,1,1,0.1)
        }
    }

    // 刻度环
    Repeater {
        model: 72
        Rectangle {
            width: index % 6 === 0 ? 3 : 2
            height: index % 6 === 0 ? 15 : 10
            color: index % 6 === 0 ? "#e6e6e6" : "#666666"
            rotation: index * 5 - 135
            x: gaugeRoot.width / 2 - width / 2
            y: 10
            transformOrigin: Item.Bottom
        }
    }

    // 刻度值
    Repeater {
        model: 9
        Text {
            text: Math.round(minValue + (index * (maxValue - minValue) / 8))
            color: "#cccccc"
            font.pixelSize: 14
            font.bold: true
            rotation: index * 45 - 135
            x: gaugeRoot.width / 2 - width / 2
            y: 35
            transformOrigin: Item.Bottom
        }
    }

    // 指针
    Rectangle {
        id: needle
        width: 4
        height: gaugeRoot.height / 2 - 40
        color: gaugeColor
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.verticalCenter
        antialiasing: true
        transformOrigin: Item.Bottom

        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: gaugeColor
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
        }

        rotation: -135 + (value - minValue) / (maxValue - minValue) * 270
        Behavior on rotation {
            NumberAnimation { duration: 500; easing.type: Easing.OutCubic }
        }
    }

    // 中央显示
    Column {
        anchors.centerIn: parent
        spacing: 5

        Text {
            text: title
            color: "#cccccc"
            font.pixelSize: 18
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            visible: showDigital
            text: value.toFixed(2)
            color: "#ffffff"
            font.pixelSize: 32
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            visible: showDigital
            text: unit
            color: gaugeColor
            font.pixelSize: 16
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    // 发光效果
    Glow {
        source: needle
        anchors.fill: needle
        radius: 8
        samples: 17
        color: gaugeColor
        spread: 0.2
    }
}
