import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: homePage

    signal toSettingPage(int value)
    signal toSystemPage

    property bool is_RemoteModel: false
    property real home_ch1cvChange: 0.0
    property real home_ch1ccChange: 1.0
    property real home_ch1ovChange: 8.0
    property real home_ch2cvChange: 0.0
    property real home_ch2ccChange: 1.0
    property real home_ch2ovChange: 8.0

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"
        visible: true

        Row {
            anchors.centerIn: parent
            anchors.margins: 10
            spacing: 150

            DigitalCard {
                id: channel_1
                channelName: "CH1"
                scale: 1.6
                cvSetpoint: homePage.home_ch1cvChange
                ccSetpoint: homePage.home_ch1ccChange
                ovSetpoint: homePage.home_ch1ovChange
                voltage: Uart_bridge.ch1_Voltage
                current: Uart_bridge.ch1_Current
                selectedMode: Uart_bridge.ch1_status_v
                unitChanged: Uart_bridge.ch1_Current_Unit
                is_enclick: !homePage.is_RemoteModel

                onClicked: {
                    Uart_bridge.setChannel_Output(1, channel_1.channelEnabled)
                }

                onDoLongPressAction: {
                    toSettingPage(1)
                }
            }

            DigitalCard {
                id: channel_2
                channelName: "CH2"
                scale: 1.6
                cvSetpoint: homePage.home_ch2cvChange
                ccSetpoint: homePage.home_ch2ccChange
                ovSetpoint: homePage.home_ch2ovChange
                voltage: Uart_bridge.ch2_Voltage
                current: Uart_bridge.ch2_Current
                selectedMode: Uart_bridge.ch2_status_v
                unitChanged: Uart_bridge.ch2_Current_Unit
                is_enclick: !homePage.is_RemoteModel

                onClicked: {
                    Uart_bridge.setChannel_Output(2, channel_2.channelEnabled)
                }

                onDoLongPressAction: {
                    toSettingPage(2)
                }
            }

            Column {
                anchors.margins: 0
                spacing: 15

                SetBox {
                    id: system_box
                    scale: 1.1
                    mainText: "System"
                    is_enclick: !homePage.is_RemoteModel
                    onClicked: {
                        toSystemPage()
                    }
                }

                SetBox {
                    id: setting_box
                    scale: 1.1
                    mainText: "Setting"
                    is_enclick: !homePage.is_RemoteModel

                    onClicked: {
                        toSettingPage(0)
                    }
                }

                SetBox {
                    id: all_s_box
                    scale: 1.1
                    mainText: "All"
                    subText: all_s_box.is_subTcolor_s ? "OFF" : "ON"
                    is_enclick: !homePage.is_RemoteModel

                    onClicked: {
                        channel_1.channelEnabled = !channel_1.channelEnabled
                        channel_2.channelEnabled = !channel_2.channelEnabled

                        all_s_box.is_subTcolor_s = !all_s_box.is_subTcolor_s
                        Uart_bridge.setChannel_Output(0,
                                                      all_s_box.is_subTcolor_s)
                    }
                }

                SetBox {
                    id: unit_box
                    scale: 1.1
                    mainText: "Unit"
                    subText: "A"
                    is_enclick: !homePage.is_RemoteModel

                    onClicked: {
                        unit_box.subText = Uart_bridge.setChannel_CurrentUnit()
                    }
                }

                SetBox {
                    id: model_box
                    scale: 1.1
                    mainText: "Model"
                    subText: "Local" // switching model_remote

                    onClicked: {
                        homePage.is_RemoteModel = !homePage.is_RemoteModel
                        model_box.is_subTcolor_s = !model_box.is_subTcolor_s
                        model_box.subText = homePage.is_RemoteModel ? "Remote" : "Local"
                    }
                }
            }
        }
    }
}
