// Setting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: settingPage

    signal backRequested

    property int initialChannel: 0

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 3

            DigitalCard {
                id: channel
                Layout.fillHeight: true
                Layout.fillWidth: true
                enclick: !Uart_bridge.isRemote
                channelOutput: settingPage.initialChannel
                               !== 0 ? Uart_bridge["ch" + settingPage.initialChannel
                                                   + "_isOutput"] : false
                channelName: settingPage.initialChannel
                             === 0 ? "CH*" : "CH" + settingPage.initialChannel
                voltage: settingPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_Voltage"] : 0.0
                current: settingPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_Current"] : 0.0
                voltageUnit: "V"
                currentUnit: settingPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + settingPage.initialChannel
                                                 + "_CurrentUnit"] : "A"
                cvSetpoint: settingPage.initialChannel
                            !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_cv"] : 0.0
                ccSetpoint: settingPage.initialChannel
                            !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_cc"] : 1.0
                ovpSetpoint: settingPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_ovp"] : 8.0
                cvModel: settingPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_Status"].charAt(
                                     14) === "1" : false
                ccModel: settingPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_Status"].charAt(
                                     13) === "1" : false
                ovpModel: settingPage.initialChannel
                          !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_Status"].charAt(
                                      11) === "1" : false

                onClicked: {
                    Uart_bridge.setChannel_Output(settingPage.initialChannel,
                                                  !channelOutput)
                    if (settingPage.initialChannel === 0) {
                        channelOutput = !channelOutput
                    }
                }


                /*onPressAndHold: {
                    if (channelOutput) {
                    } else {
                    }---用于扩展功能
                }*/
            }

            KeyinputBox {
                id: keyinputset
                enclick: !Uart_bridge.isRemote
                Layout.fillHeight: true
                Layout.fillWidth: true

                onEntervalue: {
                    //console.log("设置通道状态：" + value)
                    Uart_bridge.setChannel_Setstatus(
                                settingPage.initialChannel,
                                groupsetbox.currentsetmodel, value)
                }
            }

            SetBoxGroup {
                id: groupsetbox
                Layout.fillHeight: true
                Layout.fillWidth: true
                enclick: !Uart_bridge.isRemote
                property int currentsetmodel: 0

                box1_mainText: "CV"
                box1_subText: ""
                box1_pressed: true
                onBox1Clicked: {
                    currentsetmodel = 0
                    box1_pressed = true
                    box2_pressed = false
                    box3_pressed = false
                }

                box2_mainText: "CC"
                box2_subText: ""
                box2_pressed: false
                onBox2Clicked: {
                    currentsetmodel = 1
                    box1_pressed = false
                    box2_pressed = true
                    box3_pressed = false
                }

                box3_mainText: "OVP"
                box3_subText: ""
                box3_pressed: false
                onBox3Clicked: {
                    currentsetmodel = 3
                    box1_pressed = false
                    box2_pressed = false
                    box3_pressed = true
                }

                box4_mainText: "EXIT"
                box4_subText: ""
                onBox4Clicked: {
                    backRequested()
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
    D{i:0;autoSize:true;formeditorZoom:0.75;height:480;width:640}
}
##^##*/

