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
                    selected: settingPage.currentsetmodel === 1

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
                    selected: settingPage.currentsetmodel === 2

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
                    selected: settingPage.currentsetmodel === 3

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 3
                        keyinput.text = subText
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All"
                    property bool output: false
                    subText: output ? "OFF" : "ON"
                    subTextColor: output ? "#C06A6A" : "#8CCF6A"

                    enclick: settingPage.enclick
                    onClicked: {
                        output = !output
                        keyinput.text = ""
                        settingPage.currentsetmodel = 4
                        Uart_bridge.setChannel_Output(0, output)
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "LAN-SM"
                    subText: Uart_bridge.SM
                    selected: settingPage.currentsetmodel === 5

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
                    subText: text + " %"
                    property string text: Uart_bridge.ch1_CurrentSOC + ""
                    selected: settingPage.currentsetmodel === 6

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 6
                        keyinput.text = text
                    }
                }
                SetBox {
                    id: capacity
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-Capacity"
                    subText: text + " Ah"
                    property string text: Uart_bridge.ch1_CapacityAH + ""
                    selected: settingPage.currentsetmodel === 7

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 7
                        keyinput.text = text
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-Model"
                    subText: Uart_bridge.ch1_BatteryModel // only init value
                    subTextColor: "#6AA8B0"

                    enclick: settingPage.enclick
                    onClicked: {
                        keyinput.text = ""
                        settingPage.currentsetmodel = 8
                        subText = Uart_bridge.setChannel_BatteryModel(0)
                    }
                }

                SetBox {
                    id: cv
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-CV"
                    subText: text + " V"
                    property string text: Uart_bridge.ch1_cv + ""
                    selected: settingPage.currentsetmodel === 9

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 9
                        keyinput.text = text
                    }
                }
                SetBox {
                    id: cc
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-CC"
                    subText: text + " A"
                    property string text: Uart_bridge.ch1_cc + "" // only init value
                    selected: settingPage.currentsetmodel === 10

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 10
                        keyinput.text = text
                    }
                }
                SetBox {
                    id: ovp
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-OVP"
                    subText: text + " V"
                    property string text: Uart_bridge.ch1_ovp + ""
                    selected: settingPage.currentsetmodel === 11

                    enclick: settingPage.enclick
                    onClicked: {
                        settingPage.currentsetmodel = 11
                        keyinput.text = text
                    }
                }
                SetBox {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    mainText: "All-Unit"
                    subText: Uart_bridge.ch1_CurrentUnit // only init value
                    subTextColor: "#9A6AB0"

                    enclick: settingPage.enclick
                    onClicked: {
                        keyinput.text = ""
                        settingPage.currentsetmodel = 12
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
                        initsoc.text = value
                        Uart_bridge.setChannel_InitSOC(0, value)
                        break
                    case 7:
                        // A-Capacity
                        capacity.text = value
                        Uart_bridge.setChannel_Capacity(0, value)
                        break
                    case 9:
                        // A-CV
                        cv.text = value
                        Uart_bridge.setChannel_Setstatus(0, 0, value)
                        break
                    case 10:
                        // A-CC
                        cc.text = value
                        Uart_bridge.setChannel_Setstatus(0, 1, value)
                        break
                    case 11:
                        // A-OVP
                        ovp.text = value
                        Uart_bridge.setChannel_Setstatus(0, 3, value)
                        break
                    default:
                        console.log("Unknown mode:",
                                    settingPage.currentsetmodel)
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
        height: 18

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
        height: 18

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

