// Setting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: settingPage

    signal backRequested

    property color backgroundcolor: "#0d1b2a" //"#0a0f1a"
    property int currentsetmodel: 0

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1.8

            Rectangle {
                id: background
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 70
                color: "#0A1929"

                GridLayout {
                    anchors.fill: parent
                    columnSpacing: 1.8
                    rowSpacing: 1.8
                    columns: 4
                    rows: 3

                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "LAN-IP"
                        subText: Uart_bridge.IPaddress
                        pressed: settingPage.currentsetmodel === 1

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 1
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "GPIB-ID"
                        subText: Uart_bridge.GPIBid
                        pressed: settingPage.currentsetmodel === 2

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 2
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "CAN-ID"
                        subText: Uart_bridge.CANid
                        pressed: settingPage.currentsetmodel === 3

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 3
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        property bool Output: false

                        mainText: "A-OutPut"
                        subText: Output ? "OFF" : "ON"
                        subTextColor: Output ? "#B06A6A" : "#8CAF6A"

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            Output = !Output
                            Uart_bridge.setChannel_Output(0, Output)
                        }
                    }
                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "LAN-SM"
                        subText: Uart_bridge.SM
                        pressed: settingPage.currentsetmodel === 5

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 5
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        id: initsoc
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-InitSOC"
                        subText: "100.00 %"
                        pressed: settingPage.currentsetmodel === 6

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 6
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        id: capacity
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-Capacity"
                        subText: "00.00 Ah"
                        pressed: settingPage.currentsetmodel === 7

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 7
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-Model"
                        subText: "Model-1"
                        subTextColor: "#6AA8B0"

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            subText = Uart_bridge.setChannel_BatteryModel(0)
                        }
                    }

                    SetBox {
                        id: cv
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-CV"
                        subText: "0.000 V"
                        pressed: settingPage.currentsetmodel === 9

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 9
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        id: cc
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-CC"
                        subText: "1.000 A"
                        pressed: settingPage.currentsetmodel === 10

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 10
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        id: ovp
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-OVP"
                        subText: "8.000 V"
                        pressed: settingPage.currentsetmodel === 11

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            settingPage.currentsetmodel = 11
                            keyinput.text = subText
                        }
                    }
                    SetBox {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        mainText: "A-I-Unit"
                        subText: "A"
                        subTextColor: "#9A6AB0"

                        enclick: !Uart_bridge.isRemote
                        onClicked: {
                            subText = Uart_bridge.setChannel_CurrentUnit(0)
                        }
                    }
                }
            }

            KeyinputBox {
                id: keyinput
                enclick: !Uart_bridge.isRemote
                Layout.preferredWidth: 30
                Layout.fillHeight: true
                Layout.fillWidth: true

                onEntervalue: {
                    switch (settingPage.currentsetmodel) {
                    case 1:
                        // LAN-IP automatic update binding
                        Uart_bridge.update_Configuration(0, value)
                        break
                    case 2:
                        // GPIB-ID automatic update binding
                        Uart_bridge.update_Configuration(2, value)
                        break
                    case 3:
                        // CAN-ID automatic update binding
                        Uart_bridge.update_Configuration(3, value)
                        break
                    case 5:
                        // LAN-SM automatic update binding
                        Uart_bridge.update_Configuration(1, value)
                        break
                    case 6:
                        // A-InitSOC
                        initsoc.subText = value
                        Uart_bridge.setChannel_InitSOC(0, value)
                        break
                    case 7:
                        // A-Capacity
                        capacity.subText = value
                        Uart_bridge.setChannel_Capacity(0, value)
                        break
                    case 9:
                        // A-CV
                        cv.subText = value
                        Uart_bridge.setChannel_Setstatus(0, 0, value)
                        break
                    case 10:
                        // A-CC
                        cc.subText = value
                        Uart_bridge.setChannel_Setstatus(0, 1, value)
                        break
                    case 11:
                        // A-OVP
                        ovp.subText = value
                        Uart_bridge.setChannel_Setstatus(0, 3, value)
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
    D{i:0;autoSize:true;formeditorZoom:0.75;height:360;width:960}
}
##^##*/

