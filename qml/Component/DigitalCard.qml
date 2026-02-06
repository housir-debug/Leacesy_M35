import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 240
    height: 360

    signal clicked
    signal doLongPressAction

    property real voltage: 0.0
    property real current: 0.0
    property real cvSetpoint: 0.0
    property real ccSetpoint: 1.0
    property real ovSetpoint: 8.0
    property string channelName: "CH1"
    property bool unitChanged: false
    property string voltageUnit: "V"
    property string currentUnit: unitChanged ? "mA" : "A"
    property bool is_enclick: true
    property bool is_pressed: false
    property bool channelEnabled: false
    property int selectedMode: 0 // 1:CV, 2:CC, 3:OV
    property real scale: 1.0
    transform: Scale {
        origin.x: 0
        origin.y: 0
        xScale: root.scale
        yScale: root.scale
    }

    Rectangle {
        id: card
        radius: 36
        anchors.fill: parent
        border.width: 8
        border.color: root.is_pressed ? "#6f6669" : "#dfe6e9"
        color: root.channelEnabled ? "#cccccc" : "#9a9a9a"

        Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 18

            Item {
                width: parent.width
                height: parent.height * 0.18

                Text {
                    id: channelText
                    color: "#2c3e50"
                    font.bold: true
                    font.pixelSize: 36
                    anchors.centerIn: parent
                    text: root.channelName
                }

                Rectangle {
                    id: channelDot
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                    }
                    width: 24
                    height: 24
                    radius: width / 2
                    color: root.channelEnabled ? "#2ecc71" : "#ecf0f1"
                }
            }

            Rectangle {
                id: measurementCard
                radius: 12
                color: "#121218"
                border.width: 2
                border.color: "#901010"
                width: parent.width
                height: parent.height * 0.4

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Text {
                        id: voltageText
                        color: "#27ae60"
                        font.bold: true
                        font.pixelSize: 36
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root.voltage.toFixed(4) + " " + root.voltageUnit
                    }

                    Text {
                        id: currentText
                        color: "#e74c3c"
                        font.bold: true
                        font.pixelSize: root.unitChanged ? 24 : 36
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root.current.toFixed(4) + " " + root.currentUnit
                    }
                }
            }

            Column {
                id: setpointsColumn
                width: parent.width
                height: parent.height * 0.3
                spacing: 3

                Item {
                    id: cvRow
                    width: parent.width * 0.96
                    height: parent.height * 0.3
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: true

                    Text {
                        text: "CV"
                        color: "#0f0f0f"
                        font.bold: true
                        font.pixelSize: 18
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.cvSetpoint.toFixed(3) + " V"
                        anchors.centerIn: parent
                        color: "#04192e"
                        font.pixelSize: 14
                    }

                    Rectangle {
                        width: 18
                        height: 18
                        radius: width / 2
                        border.width: 1
                        border.color: "#666"
                        color: selectedMode === 1 ? "#e74c3c" : "#ecf0f1"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item {
                    id: ccRow
                    width: parent.width * 0.96
                    height: parent.height * 0.3
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: true

                    Text {
                        text: "CC"
                        color: "#0f0f0f"
                        font.bold: true
                        font.pixelSize: 18
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.ccSetpoint.toFixed(3) + " A"
                        anchors.centerIn: parent
                        color: "#04192e"
                        font.pixelSize: 14
                    }

                    Rectangle {
                        width: 18
                        height: 18
                        radius: width / 2
                        border.width: 1
                        border.color: "#666"
                        color: selectedMode === 2 ? "#e74c3c" : "#ecf0f1"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item {
                    id: ovRow
                    width: parent.width * 0.96
                    height: parent.height * 0.3
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: true

                    Text {
                        text: "OV"
                        color: "#0f0f0f"
                        font.bold: true
                        font.pixelSize: 18
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.ovSetpoint.toFixed(3) + " V"
                        anchors.centerIn: parent
                        color: "#04192e"
                        font.pixelSize: 14
                    }

                    Rectangle {
                        width: 18
                        height: 18
                        radius: width / 2
                        border.width: 1
                        border.color: "#666"
                        color: selectedMode === 3 ? "#e74c3c" : "#ecf0f1"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            enabled: root.is_enclick
            pressAndHoldInterval: 1000

            onPressed: {
                root.is_pressed = true
                pressAnimation.start()
            }

            onPressAndHold: {
                //console.log("长按1秒触发")  // 不会再触发后续事件
                root.is_pressed = false
                releaseAnimation.start()

                if (!root.channelEnabled) {
                    root.doLongPressAction()
                }
            }

            onReleased: {
                root.is_pressed = false
                releaseAnimation.start()
                root.channelEnabled = !root.channelEnabled
                root.clicked()
            }
        }
    }

    ScaleAnimator {
        id: pressAnimation
        target: card
        from: 1.0
        to: 0.99
        duration: 66
        easing.type: Easing.OutCubic
    }

    ScaleAnimator {
        id: releaseAnimation
        target: card
        from: 0.99
        to: 1.0
        duration: 66
        easing.type: Easing.OutCubic
    }
}
