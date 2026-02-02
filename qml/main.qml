import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    //ApplicationWindow
    id: mainWindow
    width: 1024
    height: 800
    visible: true
    property bool is_RemoteModel: false

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        Row {
            anchors.centerIn: parent
            anchors.margins: 10
            spacing: 150

            DigitalCard {
                id: channel_1
                channelName: "CH1"
                scale: 1.6
                voltage: Uart_bridge.ch1_Voltage
                current: Uart_bridge.ch1_Current
                selectedMode: Uart_bridge.ch1_status_v
                unitChanged: Uart_bridge.ch1_Current_Unit
                is_enclick: !mainWindow.is_RemoteModel

                onClicked: {
                    Uart_bridge.onChannel_1_Toggled(channel_1.channelEnabled)
                }
            }

            DigitalCard {
                id: channel_2
                channelName: "CH2"
                scale: 1.6
                voltage: Uart_bridge.ch2_Voltage
                current: Uart_bridge.ch2_Current
                selectedMode: Uart_bridge.ch2_status_v
                unitChanged: Uart_bridge.ch2_Current_Unit
                is_enclick: !mainWindow.is_RemoteModel

                onClicked: {
                    Uart_bridge.onChannel_2_Toggled(channel_2.channelEnabled)
                }
            }

            Column {
                anchors.margins: 0
                spacing: 15

                SetBox {
                    id: system_box
                    scale: 1.1
                    mainText: "System"
                    is_enclick: !mainWindow.is_RemoteModel
                }

                SetBox {
                    id: setting_box
                    scale: 1.1
                    mainText: "Setting"
                    is_enclick: !mainWindow.is_RemoteModel
                }

                SetBox {
                    id: all_s_box
                    scale: 1.1
                    mainText: "All"
                    subText: "ON"
                    is_enclick: !mainWindow.is_RemoteModel

                    onClicked: {
                        channel_1.channelEnabled = !channel_1.channelEnabled
                        channel_2.channelEnabled = !channel_2.channelEnabled

                        all_s_box.is_subTcolor_s = !all_s_box.is_subTcolor_s
                        all_s_box.subText = Uart_bridge.onAll_Channel_Change(
                                    all_s_box.is_subTcolor_s)
                    }
                }

                SetBox {
                    id: unit_box
                    scale: 1.1
                    mainText: "Unit"
                    subText: "A"
                    is_enclick: !mainWindow.is_RemoteModel

                    onClicked: {
                        unit_box.subText = Uart_bridge.onCurrent_Unit_Change()
                    }
                }

                SetBox {
                    id: model_box
                    scale: 1.1
                    mainText: "Model"
                    subText: "Local" // switching model_remote

                    onClicked: {
                        mainWindow.is_RemoteModel = !mainWindow.is_RemoteModel
                        model_box.is_subTcolor_s = !model_box.is_subTcolor_s
                        model_box.subText = mainWindow.is_RemoteModel ? "Remote" : "Local"
                    }
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}
}
##^##*/

