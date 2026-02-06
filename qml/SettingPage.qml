// Setting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: settingPage

    signal backRequested(real cvV, real ccV, real ovV)

    property int initialChannel: 0
    property int currentsetmodel: 1
    property real settingcvpoint: 0.0
    property real settingccpoint: 1.0
    property real settingovpoint: 8.0

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        Row {
            anchors.centerIn: parent
            anchors.margins: 10
            spacing: 150

            DigitalCard {
                id: channel
                channelName: settingPage.initialChannel
                             === 0 ? "CH*" : "CH" + settingPage.initialChannel
                scale: 1.6
                cvSetpoint: settingPage.settingcvpoint
                ccSetpoint: settingPage.settingccpoint
                ovSetpoint: settingPage.settingovpoint
                voltage: getVoltage()
                current: getCurrent()
                selectedMode: getSelectedMode()
                unitChanged: getCurrentUnit()
                is_enclick: false
                visible: true
            }

            KeyinputBox {
                scale: 1.6

                onEntervalue: {
                    //console.log("设置通道状态：" + value)
                    Uart_bridge.setChannel_Setstatus(
                                settingPage.initialChannel,
                                settingPage.currentsetmodel, value)

                    setCardStatusValue(value)
                }
            }

            Column {
                anchors.margins: 0
                spacing: 15
                visible: true

                SetBox {
                    id: cv_box
                    scale: 1.1
                    mainText: "CV"
                    is_pressed: true

                    onClicked: {
                        previousmodelclose()
                        settingPage.currentsetmodel = 1
                        cv_box.is_pressed = true
                    }
                }

                SetBox {
                    id: cc_box
                    scale: 1.1
                    mainText: "CC"

                    onClicked: {
                        previousmodelclose()
                        settingPage.currentsetmodel = 2
                        cc_box.is_pressed = true
                    }
                }

                SetBox {
                    id: ovp_box
                    scale: 1.1
                    mainText: "OVP"

                    onClicked: {
                        previousmodelclose()
                        settingPage.currentsetmodel = 3
                        ovp_box.is_pressed = true
                    }
                }

                SetBox {
                    id: on_box
                    scale: 1.1
                    mainText: settingPage.initialChannel === 0 ? "All" : ""
                    subText: on_box.is_subTcolor_s ? "OFF" : "ON"

                    onClicked: {
                        on_box.is_subTcolor_s = !on_box.is_subTcolor_s
                        Uart_bridge.setChannel_Output(
                                    settingPage.initialChannel,
                                    on_box.is_subTcolor_s)
                    }
                }

                SetBox {
                    id: exit_box
                    scale: 1.1
                    mainText: "EXIT"

                    onClicked: {
                        settingPage.backRequested(settingPage.settingcvpoint,
                                                  settingPage.settingccpoint,
                                                  settingPage.settingovpoint)
                    }
                }
            }
        }
    }

    function setCardStatusValue(value) {
        switch (settingPage.currentsetmodel) {
        case 1:
            if (value > 21) {
                settingPage.settingcvpoint = 21
                return
            }
            settingPage.settingcvpoint = value
            return
        case 2:
            if (value > 11) {
                settingPage.settingccpoint = 11
                return
            }
            settingPage.settingccpoint = value
            return
        case 3:
            if (value > 8) {
                settingPage.settingovpoint = 8
                return
            }
            settingPage.settingovpoint = value
            return
        default:
            return
        }
    }

    function previousmodelclose() {
        switch (settingPage.currentsetmodel) {
        case 1:
            cv_box.is_pressed = false
            return
        case 2:
            cc_box.is_pressed = false
            return
        case 3:
            ovp_box.is_pressed = false
            return
        default:
            return
        }
    }

    function getVoltage() {
        switch (settingPage.initialChannel) {
        case 1:
            return Uart_bridge.ch1_Voltage
        case 2:
            return Uart_bridge.ch2_Voltage
        case 3:
            return Uart_bridge.ch3_Voltage
        case 4:
            return Uart_bridge.ch4_Voltage
        default:
            return 0
        }
    }

    function getCurrent() {
        switch (settingPage.initialChannel) {
        case 1:
            return Uart_bridge.ch1_Current
        case 2:
            return Uart_bridge.ch2_Current
        case 3:
            return Uart_bridge.ch3_Current
        case 4:
            return Uart_bridge.ch4_Current
        default:
            return 0
        }
    }

    function getSelectedMode() {
        switch (settingPage.initialChannel) {
        case 1:
            return Uart_bridge.ch1_status_v
        case 2:
            return Uart_bridge.ch2_status_v
        case 3:
            return Uart_bridge.ch3_status_v
        case 4:
            return Uart_bridge.ch4_status_v
        default:
            return 0
        }
    }

    function getCurrentUnit() {
        switch (settingPage.initialChannel) {
        case 1:
            return Uart_bridge.ch1_Current_Unit
        case 2:
            return Uart_bridge.ch2_Current_Unit
        case 3:
            return Uart_bridge.ch3_Current_Unit
        case 4:
            return Uart_bridge.ch4_Current_Unit
        default:
            return "A"
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.75;height:480;width:640}
}
##^##*/

