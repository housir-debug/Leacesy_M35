// Setting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: settingPage

    signal toDigitalHomePage
    signal toBatteryHomePage

    property bool enclick: true
    property int currentsetmodel: 0
    property color backgroundcolor: "#0d1b2a"

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 6.6

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 70
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

                    enclick: settingPage.enclick
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

                    enclick: settingPage.enclick
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

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 3
                        keyinput.text = subText
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    property bool output: false

                    mainText: "All"
                    subText: output ? "OFF" : "ON"
                    subTextColor: output ? "#B06A6A" : "#8CAF6A"

                    enclick: settingPage.enclick
                    onClicked: {
                        output = !output
                        Uart_bridge.setChannel_Output(0, output)
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "LAN-SM"
                    subText: Uart_bridge.SM
                    pressed: settingPage.currentsetmodel === 5

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 5
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: initsoc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-SOC"
                    subText: "100.00 %"
                    pressed: settingPage.currentsetmodel === 6

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 6
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: capacity
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-Ah"
                    subText: "0.00"
                    pressed: settingPage.currentsetmodel === 7

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 7
                        keyinput.text = subText
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-Model"
                    subText: "Lithium"
                    subTextColor: "#6AA8B0"

                    enclick: settingPage.enclick
                    onClicked: {
                        subText = Uart_bridge.setChannel_BatteryModel(0)
                    }
                }

                SetBox {
                    id: cv
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-CV"
                    subText: "0.000 V"
                    pressed: settingPage.currentsetmodel === 9

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 9
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: cc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-CC"
                    subText: "0.000 A"
                    pressed: settingPage.currentsetmodel === 10

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 10
                        keyinput.text = subText
                    }
                }
                SetBox {
                    id: ovp
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-OVP"
                    subText: "0.000 V"
                    pressed: settingPage.currentsetmodel === 11

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 11
                        keyinput.text = subText
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-Unit"
                    subText: "A"
                    subTextColor: "#9A6AB0"

                    enclick: settingPage.enclick
                    onClicked: {
                        subText = Uart_bridge.setChannel_CurrentUnit(0)
                    }
                }
            }

            KeyinputBox {
                id: keyinput
                enclick: settingPage.enclick
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
                        initsoc.subText = value + " %"
                        Uart_bridge.setChannel_InitSOC(0, value)
                        break
                    case 7:
                        // A-Capacity
                        capacity.subText = value
                        Uart_bridge.setChannel_Capacity(0, value)
                        break
                    case 9:
                        // A-CV
                        cv.subText = value + " V"
                        Uart_bridge.setChannel_Setstatus(0, 0, value)
                        break
                    case 10:
                        // A-CC
                        cc.subText = value + " A"
                        Uart_bridge.setChannel_Setstatus(0, 1, value)
                        break
                    case 11:
                        // A-OVP
                        ovp.subText = value + " V"
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
        id: bottomEdgeSwipe
        enabled: settingPage.enclick
        anchors.bottom: parent.bottom
        width: parent.width
        height: 36

        property real startY: 0
        onPressed: startY = mouseY
        onReleased: {
            if (startY - mouseY > 81) {
                toDigitalHomePage()
            }
        }
    }

    MouseArea {
        id: topEdgeSwipe
        enabled: settingPage.enclick
        anchors.top: parent.top
        width: parent.width
        height: 36

        property real startY: 0
        onPressed: startY = mouseY
        onReleased: {
            if (mouseY - startY > 81) {
                toBatteryHomePage()
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.75;height:360;width:960}
}
##^##*/

