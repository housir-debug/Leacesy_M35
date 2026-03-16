import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: homePage

    signal toSystemPage
    signal toSettingPage(int value)

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        Row {
            anchors.fill: parent

            GridLayout {
                id: channelsGrid
                width: parent.width - groupsetbox.width
                height: parent.height
                columns: 9
                rows: 4
                columnSpacing: 1
                rowSpacing: 1

                Repeater {
                    model: 36 // channel count
                    delegate: DigitalCard {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        enclick: !Uart_bridge.isRemote
                        channelOutput: false
                        channelName: "CH" + (index + 1)
                        voltage: Uart_bridge["ch" + (index + 1) + "_Voltage"]
                        current: Uart_bridge["ch" + (index + 1) + "_Current"]
                        voltageUnit: "V"
                        currentUnit: Uart_bridge["ch" + (index + 1) + "_CurrentUnit"]
                        cvSetpoint: Uart_bridge["ch" + (index + 1) + "_cv"]
                        ccSetpoint: Uart_bridge["ch" + (index + 1) + "_cc"]
                        ovpSetpoint: Uart_bridge["ch" + (index + 1) + "_ovp"]
                        cvModel: Uart_bridge["ch" + (index + 1) + "_Status"].charAt(
                                     14) === "1"
                        ccModel: Uart_bridge["ch" + (index + 1) + "_Status"].charAt(
                                     13) === "1"
                        ovpModel: Uart_bridge["ch" + (index + 1) + "_Status"].charAt(
                                      11) === "1"

                        onClicked: {
                            channelOutput = !channelOutput
                            Uart_bridge.setChannel_Output(index + 1,
                                                          channelOutput)
                        }

                        onPressAndHold: {
                            if (channelOutput) {

                                //输出详情页，还没开放
                            } else {
                                toSettingPage(index + 1)
                            }
                        }
                    }
                }
            }

            SetBoxGroup {
                id: groupsetbox
                scaleFactor: parent.height / implicitHeight
                enclick: !Uart_bridge.isRemote

                box1_mainText: "System"
                box1_subText: ""
                onBox1Clicked: toSystemPage()

                box2_mainText: "All"
                box2_subText: "Setting"
                onBox2Clicked: toSettingPage(0)

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
                onBox5Clicked: {
                    box5_enclick = true
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

