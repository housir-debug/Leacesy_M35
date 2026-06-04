// Function.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: functionPage

    signal backRequested

    property bool enclick: true
    property int initialChannel: 1
    property int currentsetmodel: 0
    property color backgroundcolor: "#0d1b2a"

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 6.6

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 69

                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: backgroundcolor

                    Text {
                        text: "Software"
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "#1E3A5F"

                    Text {
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                        text: "V: " + Uart_bridge.SoftVer
                    }
                }

                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: backgroundcolor

                    Text {
                        color: "#e0e0e0"
                        text: "Hardware"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "#1E3A5F"

                    Text {
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                        text: "V: " + Uart_bridge.HardVer
                    }
                }

                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: backgroundcolor

                    Text {
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        text: "CH-Software"
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "#1E3A5F"

                    Text {
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                        text: "V: " + Uart_bridge["ch" + functionPage.initialChannel + "_sv"]
                    }
                }

                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: backgroundcolor

                    Text {
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        text: "CH-Hardware"
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "#1E3A5F"

                    Text {
                        color: "#e0e0e0"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                        text: "V: " + Uart_bridge["ch" + functionPage.initialChannel + "_hv"]
                    }
                }
            }

            Rectangle {
                id: statusIndicator
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 99
                color: functionPage.backgroundcolor

                readonly property var statusLabels: ["OPC", "WAI", "LCC", "ERR", "CP", "FAN", "CAL", "RL", "IMP", "OT", "OC", "OVP", "LD", "CC", "CV", "PON"]

                GridLayout {
                    anchors.fill: parent
                    columns: 4
                    rows: 4

                    Repeater {
                        model: 16
                        delegate: Rectangle {
                            id: indicator
                            radius: 9
                            border.width: 1.8
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: isActive ? "#0F2847" : "#0A1929"
                            border.color: isActive ? "#1E5799" : "#1E3A5F"

                            required property int index
                            property bool isActive: functionPage.initialChannel
                                                    !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Status"].charAt(
                                                                index) === "1" : false
                            Text {
                                font.bold: true
                                anchors.centerIn: parent
                                text: statusIndicator.statusLabels[index]
                                color: indicator.isActive ? "#CCCCCC" : "#6a9aaf"
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius

                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: Qt.rgba(
                                                   1, 1, 1,
                                                   indicator.isActive ? 0.18 : 0.09)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: "transparent"
                                    }
                                }
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

                enclick: functionPage.enclick
                channelName: "CH" + functionPage.initialChannel
                channelOutput: Uart_bridge["ch" + functionPage.initialChannel + "_isOutput"]

                voltage: Uart_bridge["ch" + functionPage.initialChannel + "_Voltage"]
                voltageUnit: "V"
                current: Uart_bridge["ch" + functionPage.initialChannel + "_Current"]
                currentUnit: Uart_bridge["ch" + functionPage.initialChannel + "_CurrentUnit"]
                soc: Uart_bridge["ch" + functionPage.initialChannel + "_CurrentSOC"]

                onDigitalclicked: {
                    Uart_bridge.setChannel_Output(functionPage.initialChannel,
                                                  !channelOutput)
                }

                onBatteryclicked: {
                    Uart_bridge.setChannel_BatteryOutput(
                                functionPage.initialChannel, !channelOutput)
                }
            }

            GridLayout {
                Layout.preferredWidth: 99
                Layout.fillHeight: true
                Layout.fillWidth: true
                columnSpacing: 1.8
                rowSpacing: 1.8
                columns: 2
                rows: 4

                SetBox {
                    id: cv
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "CV"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_cv"] + " V"
                    selected: functionPage.currentsetmodel === 1

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 1
                        keyinput.text = Uart_bridge["ch" + functionPage.initialChannel + "_cv"]
                    }
                }
                SetBox {
                    id: initsoc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "SOC"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_CurrentSOC"] + " %"
                    selected: functionPage.currentsetmodel === 2

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 2
                        keyinput.text = Uart_bridge["ch" + functionPage.initialChannel
                                                    + "_CurrentSOC"]
                    }
                }
                SetBox {
                    id: cc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "CC"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_cc"] + " A"
                    selected: functionPage.currentsetmodel === 3

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 3
                        keyinput.text = Uart_bridge["ch" + functionPage.initialChannel + "_cc"]
                    }
                }
                SetBox {
                    id: capacity
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "Capacity"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_CapacityAH"] + " Ah"
                    selected: functionPage.currentsetmodel === 4

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 4
                        keyinput.text = Uart_bridge["ch" + functionPage.initialChannel
                                                    + "_CapacityAH"]
                    }
                }
                SetBox {
                    id: ovp
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "OVP"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_ovp"] + " V"
                    selected: functionPage.currentsetmodel === 5

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 5
                        keyinput.text = Uart_bridge["ch" + functionPage.initialChannel + "_ovp"]
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "Model"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_BatteryModel"]
                    subTextColor: "#6AA8B0"

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 6
                        Uart_bridge.setChannel_BatteryModel(
                                    functionPage.initialChannel)
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "Unit"
                    subText: Uart_bridge["ch" + functionPage.initialChannel + "_CurrentUnit"]
                    subTextColor: "#9A6AB0"

                    enclick: functionPage.enclick
                    onClicked: {
                        functionPage.currentsetmodel = 7
                        Uart_bridge.setChannel_CurrentUnit(
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

                    enclick: functionPage.enclick
                    onClicked: {
                        modeStatus = !modeStatus
                        functionPage.currentsetmodel = 8
                        Uart_bridge.setChannel_Batterymode(
                                    functionPage.initialChannel, modeStatus)
                    }
                }
            }

            KeyinputBox {
                id: keyinput
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 99
                enclick: functionPage.enclick

                onEntervalue: {
                    switch (functionPage.currentsetmodel) {
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
                        console.log("Unknown mode:",
                                    functionPage.currentsetmodel)
                        return
                    }
                }
            }
        }
    }

    MouseArea {
        id: bottomEdgeSwipe
        enabled: functionPage.enclick
        anchors.bottom: parent.bottom
        width: parent.width
        height: 18

        property real startY: 0
        onPressed: startY = mouseY
        onReleased: {
            if (startY - mouseY > 81) {
                backRequested()
            }
        }
    }

    MouseArea {
        id: topEdgeSwipe
        enabled: functionPage.enclick
        anchors.top: parent.top
        width: parent.width
        height: 18

        property real startY: 0
        onPressed: startY = mouseY
        onReleased: {
            if (mouseY - startY > 81) {
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

