// Function.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: functionPage

    signal backRequested

    property color backgroundcolor: "#0d1b2a" //"#0a0f1a"
    property int initialChannel: 0
    property int currentsetmodel: 0

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1.8

            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 66
                color: backgroundcolor

                ColumnLayout {
                    anchors.fill: parent

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "Software-Ver:"
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge.SoftVer
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "Hardware-Ver:"
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge.HardVer
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "CH-Software-Ver:"
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge["ch"
                                + (functionPage.initialChannel + 1) + "_sv"]
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "CH-Hardware-Ver:"
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge["ch"
                                + (functionPage.initialChannel + 1) + "_hv"]
                            font.pixelSize: 18
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                }
            }

            Rectangle {
                id: statusIndicator
                property real scaleFactor: 0.9
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 99
                color: backgroundcolor
                border.color: "#1a2f42"
                border.width: 1 * statusIndicator.scaleFactor
                radius: 18 * statusIndicator.scaleFactor

                readonly property color colorGlow: "#1E5799"
                readonly property var statusLabels: ["OPC", "WAI", "LCC", "ERR", "CP", "FAN", "CAL", "RL", "IMP", "OT", "OC", "OVP", "LD", "CC", "CV", "PON"]

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 8 * statusIndicator.scaleFactor
                    rowSpacing: 3 * statusIndicator.scaleFactor
                    columnSpacing: 3 * statusIndicator.scaleFactor
                    columns: 4
                    rows: 4

                    Repeater {
                        model: 16
                        delegate: Rectangle {
                            id: indicator
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            color: isActive ? "#0F2847" : "#0A1929"
                            border.color: isActive ? "#1E5799" : "#1E3A5F"
                            border.width: 1.8 * statusIndicator.scaleFactor
                            radius: 6 * statusIndicator.scaleFactor

                            required property int index
                            property bool isActive: functionPage.initialChannel
                                                    !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Status"].charAt(
                                                                index) === "1" : false
                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: parent.height / 2
                                radius: parent.radius

                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: Qt.rgba(
                                                   1, 1, 1,
                                                   indicator.isActive ? 0.2 : 0.08)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: "transparent"
                                    }
                                }
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: parent.height / 2
                                radius: parent.radius

                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: "transparent"
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: Qt.rgba(0, 0, 0, 0.3)
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius - 1.5
                                color: "transparent"
                                border.width: 1.0
                                border.color: Qt.rgba(
                                                  statusIndicator.colorGlow.r,
                                                  statusIndicator.colorGlow.g,
                                                  statusIndicator.colorGlow.b,
                                                  indicator.isActive ? 0.3 : 0.1)
                            }

                            Text {
                                anchors.centerIn: parent
                                text: statusIndicator.statusLabels[index]
                                color: indicator.isActive ? "#CCCCCC" : "#6a9aaf"
                                font.pixelSize: 18 * statusIndicator.scaleFactor
                                font.bold: true
                            }
                        }
                    }
                }
            }

            SimpleCard {
                id: channel
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 99

                enclick: !Uart_bridge.isRemote
                channelOutput: functionPage.initialChannel
                               !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                   + "_isOutput"] : false
                channelName: functionPage.initialChannel
                             === 0 ? "CH*" : "CH" + functionPage.initialChannel

                soc: functionPage.initialChannel
                     !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_CurrentSOC"] : 100.0
                voltage: functionPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Voltage"] : 0.0
                voltageUnit: "V"
                current: functionPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Current"] : 0.0
                currentUnit: functionPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                 + "_CurrentUnit"] : "A"

                onDigitalclicked: {
                    Uart_bridge.setChannel_Output(functionPage.initialChannel,
                                                  !channelOutput)
                    if (functionPage.initialChannel === 0) {
                        channelOutput = !channelOutput
                    }
                }

                onBatteryclicked: {
                    Uart_bridge.setChannel_BatteryOutput(
                                functionPage.initialChannel, !channelOutput)
                    if (functionPage.initialChannel === 0) {
                        channelOutput = !channelOutput
                    }
                }
            }

            GridLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 99
                columnSpacing: 1.8
                rowSpacing: 1.8
                columns: 2
                rows: 4

                SetBox {
                    id: cv
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "CV"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_cv"] + ""
                    pressed: functionPage.currentsetmodel === 1

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        functionPage.currentsetmodel = 1
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: initsoc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "InitSOC"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_currentSOC"] + ""
                    pressed: functionPage.currentsetmodel === 2

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        functionPage.currentsetmodel = 2
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: cc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "CC"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_cc"] + ""
                    pressed: functionPage.currentsetmodel === 3

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        functionPage.currentsetmodel = 3
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: capacity
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "Capacity"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_CapacityAH"] + ""
                    pressed: functionPage.currentsetmodel === 4

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        functionPage.currentsetmodel = 4
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: ovp
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "OVP"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_ovp"] + ""
                    pressed: functionPage.currentsetmodel === 5

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        functionPage.currentsetmodel = 5
                        keyinput.text = subText
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "Model"
                    subText: "Model-1"
                    subTextColor: "#6AA8B0"

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        subText = Uart_bridge.setChannel_BatteryModel(
                                    functionPage.initialChannel)
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "Unit"
                    subText: "A"
                    subTextColor: "#9A6AB0"

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        subText = Uart_bridge.setChannel_CurrentUnit(
                                    functionPage.initialChannel)
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    property bool modeStatus: false

                    mainText: "Mode"
                    subText: modeStatus ? "static" : "dynamic"
                    subTextColor: "#9A6AB0"

                    enclick: !Uart_bridge.isRemote
                    onClicked: {
                        modeStatus = !modeStatus
                        Uart_bridge.setChannel_Batterymode(
                                    functionPage.initialChannel, modeStatus)
                    }
                }
            }

            KeyinputBox {
                id: keyinput
                enclick: !Uart_bridge.isRemote
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 99

                onEntervalue: {
                    switch (settingPage.currentsetmodel) {
                    case 1:
                        // CV
                        Uart_bridge.setChannel_Setstatus(
                                    functionPage.initialChannel, 0, value)
                        break
                    case 2:
                        // InitSOC
                        Uart_bridge.setChannel_InitSOC(
                                    functionPage.initialChannel, value)
                        break
                    case 3:
                        // CC
                        Uart_bridge.setChannel_Setstatus(
                                    functionPage.initialChannel, 1, value)
                        break
                    case 4:
                        // Capacity
                        Uart_bridge.setChannel_Capacity(
                                    functionPage.initialChannel, value)
                        break
                    case 5:
                        // OVP
                        Uart_bridge.setChannel_Setstatus(
                                    functionPage.initialChannel, 3, value)
                        break
                    default:
                        console.log("Unknown model:", model)
                        return
                    }
                }
            }
        }
    }

    MouseArea {
        id: topEdgeSwipe
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        property real startY: 0

        onPressed: startY = mouseY
        onReleased: {
            var delta = mouseY - startY
            if (delta > 108) {
                backRequested()
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.66;height:360;width:960}
}
##^##*/

