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
                Layout.preferredWidth: 280
                Layout.preferredHeight: 400
                enclick: !Uart_bridge.isRemote
                channelOutput: false
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


                /*onClicked: {
                    channelOutput = !channelOutput
                    Uart_bridge.setChannel_Output(settingPage.initialChannel,
                                                  channelOutput)
                }

                onPressAndHold: {
                    if (channelOutput) {
                    } else {
                    }---用于扩展功能
                }*/
            }

            KeyinputBox {
                id: keyinputset
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 280
                Layout.preferredHeight: 400

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
                Layout.preferredWidth: 88
                Layout.preferredHeight: 417
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


                /*box4_mainText: settingPage.initialChannel === 0 ? "All" : ""
                box4_subText: "SOC"
                onBox4Clicked: {

                    // 待补充
                }*/
                box4_mainText: settingPage.initialChannel === 0 ? "All" : ""
                property bool allOn: false
                box4_subText: allOn ? "OFF" : "ON"
                box4_subTextColor: allOn ? "#FF3D52" : "#1DBF75"
                onBox4Clicked: {
                    allOn = !allOn
                    Uart_bridge.setChannel_Output(settingPage.initialChannel,
                                                  allOn)
                }

                box5_mainText: "EXIT"
                box5_subText: ""
                onBox5Clicked: {
                    backRequested()
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

