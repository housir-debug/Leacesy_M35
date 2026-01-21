import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 240
    height: 360

    property string channelName: "CH1"
    property real voltage: 0.0
    property real current: 0.0

    property real cvSetpoint: 0.0
    property real ccSetpoint: 1.0
    property real ovSetpoint: 8.0

    property string voltageUnit: "V"
    property string currentUnit: "A"

    property real scale: 1.0
    transform: Scale {
        origin.x: 0
        origin.y: 0
        xScale: root.scale
        yScale: root.scale
    }

    // ========== 状态属性 ==========
    property int selectedMode: 0 // 0:CV, 1:CC, 2:OV
    property bool isSelected: false
    property bool channelEnabled: false

    // ========== 动画属性 ==========
    property real scaleFactor: isSelected ? 0.98 : 1.0
    property color channelDotColor: channelEnabled ? "#2ecc71" : "#ecf0f1"
    property color cvDotColor: selectedMode === 0 ? "#e74c3c" : "#ecf0f1"
    property color ccDotColor: selectedMode === 1 ? "#e74c3c" : "#ecf0f1"
    property color ovDotColor: selectedMode === 2 ? "#e74c3c" : "#ecf0f1"

    // ========== 信号 ==========
    signal clicked
    signal modeSelected(int mode)
    signal channelToggled(bool enabled)

    // ========== 主框架 ==========
    Rectangle {
        id: card
        anchors.fill: parent

        radius: 36
        color: root.isSelected ? "#eeeeee" : "#9a9a9a"
        border.width: 8
        border.color: root.isSelected ? "#6f6669" : "#dfe6e9"

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            onClicked: {
                root.clicked()
                root.isSelected = !root.isSelected
                root.channelEnabled = !root.channelEnabled
                root.channelToggled(root.channelEnabled)

                if (root.isSelected && root.selectedMode !== 0) {
                    root.selectedMode = 0
                    root.modeSelected(0)
                }
            }

            onPressed: {
                pressAnimation.start()
            }

            onReleased: {
                releaseAnimation.start()
            }
            onEntered: {
                if (!root.isSelected) {
                    card.border.color = "#bdc3c7"
                }
            }
            onExited: {
                if (!root.isSelected) {
                    card.border.color = "#dfe6e9"
                }
            }
        }

        // ========== 显示内容 ==========
        Column {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 18
            spacing: 32

            Item {
                id: titleRow
                width: parent.width
                height: 36

                Text {
                    id: channelText
                    text: root.channelName
                    color: "#2c3e50"
                    font.pixelSize: 24
                    font.bold: true
                    font.family: "Segoe UI, Arial"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                RoundButton {
                    id: channelDot
                    width: 24
                    height: 24
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    background: Rectangle {
                        implicitWidth: 24
                        implicitHeight: 24
                        radius: width / 2
                        color: root.channelDotColor
                    }
                }
            }

            Rectangle {
                id: measurementCard
                width: parent.width
                height: 140
                radius: 12
                color: "#121212"
                border.width: 1
                border.color: "#901010"

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Text {
                        id: voltageText
                        text: (root.current >= 0 ? "" : "-") + root.voltage.toFixed(
                                  4) + " " + root.voltageUnit
                        color: "#27ae60"
                        font.pixelSize: 30
                        font.bold: true
                        font.family: "Consolas, 'Courier New', monospace"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        id: currentText
                        text: (root.current >= 0 ? "" : "-") + root.current.toFixed(
                                  4) + " " + root.currentUnit
                        color: "#e74c3c"
                        font.pixelSize: 30
                        font.bold: true
                        font.family: "Consolas, 'Courier New', monospace"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            Column {
                id: setpointsColumn
                width: parent.width
                spacing: 6

                ButtonGroup {
                    id: modeGroup
                    exclusive: true
                }

                Row {
                    id: cvRow
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 32

                    Text {
                        text: "CV"
                        color: "#0f0f0f"
                        font.pixelSize: 18
                        font.bold: true
                        width: 30
                    }

                    Text {
                        text: root.cvSetpoint.toFixed(
                                  3) + " " + root.voltageUnit
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#04192e"
                        font.pixelSize: 14
                        font.family: "Consolas, monospace"
                    }

                    RadioButton {
                        id: cvRadio
                        width: 18
                        height: 18
                        ButtonGroup.group: modeGroup
                        anchors.verticalCenter: parent.verticalCenter
                        checked: root.selectedMode === 0
                        onClicked: {
                            if (root.isSelected) {
                                root.selectedMode = 0
                                root.modeSelected(0)
                            }
                        }

                        indicator: Rectangle {
                            width: 18
                            height: 18
                            radius: width / 2
                            color: root.cvDotColor
                            border.width: 1
                            border.color: "#666"
                        }
                    }
                }

                Row {
                    id: ccRow
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 32

                    Text {
                        text: "CC"
                        color: "#0f0f0f"
                        font.pixelSize: 18
                        font.bold: true
                        width: 30
                    }

                    Text {
                        text: root.ccSetpoint.toFixed(
                                  3) + " " + root.currentUnit
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#04192e"
                        font.pixelSize: 14
                        font.family: "Consolas, monospace"
                    }

                    RadioButton {
                        id: ccRadio
                        width: 18
                        height: 18
                        ButtonGroup.group: modeGroup
                        anchors.verticalCenter: parent.verticalCenter
                        checked: root.selectedMode === 1
                        onClicked: {
                            if (root.isSelected) {
                                root.selectedMode = 1
                                root.modeSelected(1)
                            }
                        }

                        indicator: Rectangle {
                            implicitWidth: 18
                            implicitHeight: 18
                            radius: width / 2
                            color: root.ccDotColor
                            border.width: 1
                            border.color: "#666"
                        }
                    }
                }

                Row {
                    id: ovRow
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 32

                    Text {
                        text: "OV"
                        color: "#0f0f0f"
                        font.pixelSize: 18
                        font.bold: true
                        width: 30
                    }

                    Text {
                        text: root.ovSetpoint.toFixed(
                                  3) + " " + root.voltageUnit
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#04192e"
                        font.pixelSize: 14
                        font.family: "Consolas, monospace"
                    }

                    RadioButton {
                        id: ovRadio
                        width: 18
                        height: 18
                        ButtonGroup.group: modeGroup
                        anchors.verticalCenter: parent.verticalCenter
                        checked: root.selectedMode === 2
                        onClicked: {
                            if (root.isSelected) {
                                root.selectedMode = 2
                                root.modeSelected(2)
                            }
                        }

                        indicator: Rectangle {
                            implicitWidth: 18
                            implicitHeight: 18
                            radius: width / 2
                            color: root.ovDotColor
                            border.width: 1
                            border.color: "#666"
                        }
                    }
                }
            }
        }
    }

    // ========== 动画效果 ==========
    ScaleAnimator {
        id: pressAnimation
        target: card
        from: 1.0
        to: 0.99
        duration: 100
        easing.type: Easing.OutCubic
    }

    ScaleAnimator {
        id: releaseAnimation
        target: card
        from: 0.99
        to: 1.0
        duration: 100
        easing.type: Easing.OutCubic
    }

    // ========== 属性变化动画 ==========
    Behavior on scaleFactor {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    Behavior on channelDotColor {
        ColorAnimation {
            duration: 300
        }
    }

    Behavior on cvDotColor {
        ColorAnimation {
            duration: 300
        }
    }

    Behavior on ccDotColor {
        ColorAnimation {
            duration: 300
        }
    }

    Behavior on ovDotColor {
        ColorAnimation {
            duration: 300
        }
    }

    Behavior on isSelected {
        PropertyAnimation {
            properties: "border.color, color"
            duration: 300
        }
    }

    // ========== 工具函数 ==========
    function toggleChannel() {
        root.channelEnabled = !root.channelEnabled
        root.channelToggled(root.channelEnabled)
    }
}
