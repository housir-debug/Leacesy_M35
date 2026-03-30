import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: homePage

    signal toSystemPage(int value)
    signal toBatterySettingPage(int value)
    signal toFunctionPage(int value)

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        RowLayout {
            anchors.fill: parent

            GridLayout {
                id: channelsGrid
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.margins: 1.8
                columnSpacing: 1.8
                rowSpacing: 1.8
                columns: 9
                rows: 4

                Repeater {
                    model: 36 // channel count
                    delegate: BatteryBar {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        enclick: !Uart_bridge.isRemote
                        channelOutput: Uart_bridge["ch" + (index + 1) + "_isOutput"]
                        channelName: "CH" + (index + 1)
                        soc: Uart_bridge["ch" + (index + 1) + "_CurrentSOC"]
                        voltage: Uart_bridge["ch" + (index + 1) + "_Voltage"]
                        voltageUnit: "V"
                        current: Uart_bridge["ch" + (index + 1) + "_Current"]
                        currentUnit: Uart_bridge["ch" + (index + 1) + "_CurrentUnit"]
                        esr: Uart_bridge["ch" + (index + 1) + "_imp"]
                        esrUnit: "Ω"
                        batteryMode: Uart_bridge["ch" + (index + 1) + "_BatteryMode"]
                        workMode: Uart_bridge["ch" + (index + 1) + "_WorkMode"]
                        batteryCapacity: Uart_bridge["ch" + (index + 1) + "_CapacityAH"]

                        onClicked: {
                            Uart_bridge.setChannel_Output(index + 1,
                                                          !channelOutput)
                        }

                        onPressAndHold: {
                            if (channelOutput) {
                                toFunctionPage(index + 1)
                            } else {
                                toBatterySettingPage(index + 1)
                            }
                        }
                    }
                }
            }

            SetBoxGroup {
                id: groupsetbox
                Layout.fillHeight: true
                Layout.fillWidth: true
                enclick: !Uart_bridge.isRemote

                box1_mainText: "System"
                box1_subText: ""
                onBox1Clicked: toSystemPage(1)

                box2_mainText: "All"
                box2_subText: "Setting"
                onBox2Clicked: toBatterySettingPage(0)

                box3_mainText: "All"
                property bool allOn: false
                box3_subText: allOn ? "OFF" : "ON"
                box3_subTextColor: allOn ? "#FF3D52" : "#1DBF75"
                onBox3Clicked: {
                    allOn = !allOn
                    Uart_bridge.setChannel_Output(0, allOn)
                }

                box4_mainText: "Unit"
                property string currentUnit: "A"
                box4_subText: currentUnit
                onBox4Clicked: {
                    currentUnit = Uart_bridge.setChannel_CurrentUnit()
                }

                box5_mainText: "Model"
                box5_subText: Uart_bridge.isRemote ? "Remote" : "Local"
                box5_subTextColor: Uart_bridge.isRemote ? "#FF3D52" : "#1DBF75"
                box5_enclick: true
                onBox5Clicked: {
                    Uart_bridge.update_remotemodel(!Uart_bridge.isRemote)
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/

