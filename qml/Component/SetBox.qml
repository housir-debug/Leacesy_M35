import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 100
    height: 100

    property string mainText: "Setting"
    property string subText: "All"
    property real scale: 1.0
    transform: Scale {
        origin.x: 0
        origin.y: 0
        xScale: root.scale
        yScale: root.scale
    }
    // ========== 状态属性 ==========
    property bool pressed: false
    property bool hovered: false

    // ========== 信号 ==========
    signal clicked

    // ========== 主框架 ==========
    Rectangle {
        id: boxButton
        anchors.fill: parent

        radius: 18
        color: root.pressed ? "#3e444a" : "#f8f9fa"
        border.width: 6
        border.color: root.pressed ? "#8e949a" : "#ced4da"

        Column {
            anchors.centerIn: parent
            spacing: 6

            Text {
                id: subLabel
                text: root.subText
                font.pixelSize: 18
                color: "#3c454d"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.subText !== ""
            }

            Text {
                id: mainLabel
                text: root.mainText
                font.pixelSize: 18
                font.bold: true
                color: "#212529"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.subText !== ""
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: boxButton
            hoverEnabled: true

            onPressed: {
                root.pressed = true
                pressAnimation.start()
            }

            onReleased: {
                root.pressed = false
                releaseAnimation.start()
                root.clicked()
            }
        }

        // ========== 动画效果 ==========
        ScaleAnimator {
            id: pressAnimation
            target: root
            from: 1.0
            to: 0.99
            duration: 100
            easing.type: Easing.OutCubic
        }

        ScaleAnimator {
            id: releaseAnimation
            target: root
            from: 0.99
            to: 1.0
            duration: 100
            easing.type: Easing.OutBack
        }

        Behavior on border.color {
            ColorAnimation {
                duration: 150
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }
    }
}
