import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: systemPage

    signal backRequested

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 3

            StackLayout {
                id: sysStacklayout
                Layout.fillHeight: true
                Layout.fillWidth: true
                //Layout.preferredWidth: 480
                currentIndex: 0

                // index=0
                KeyinputBox {
                    id: syskeyinput
                    onEntervalue: {
                        //console.log("设置通道状态：" + value)
                        Uart_bridge.update_Configuration(
                                    groupsysbox.currentsetmodel, value)
                    }
                }

                // index=1
                SystemVersionBox {
                    softwareVersion: Uart_bridge.SoftVer
                    hardwareVersion: Uart_bridge.HardVer
                    channelSoftwareVersions: Uart_bridge.ChannelSV
                    channelHardwareVersions: Uart_bridge.ChannelHV
                }
            }

            SetBoxGroup {
                id: groupsysbox
                Layout.fillHeight: true
                Layout.fillWidth: true
                //Layout.preferredWidth: 88
                enclick: !Uart_bridge.isRemote
                property int currentsetmodel: 0

                box1_mainText: "LAN"
                property bool model: false
                box1_subText: model ? "NetMask" : "IP"
                box1_pressed: true
                onBox1Clicked: {
                    model = !model
                    currentsetmodel = model ? 1 : 0
                    syskeyinput.text = model ? Uart_bridge.SM : Uart_bridge.IPaddress
                    sysStacklayout.currentIndex = 0
                    box1_pressed = true
                    box2_pressed = false
                    box3_pressed = false
                }

                box2_mainText: "GPIB"
                box2_subText: ""
                box2_pressed: false
                onBox2Clicked: {
                    currentsetmodel = 2
                    syskeyinput.text = Uart_bridge.GPIBid
                    sysStacklayout.currentIndex = 0
                    box1_pressed = false
                    box2_pressed = true
                    box3_pressed = false
                }

                box3_mainText: "CAN"
                box3_subText: ""
                box3_pressed: false
                onBox3Clicked: {
                    syskeyinput.text = Uart_bridge.CANid
                    sysStacklayout.currentIndex = 0
                    currentsetmodel = 3
                    box1_pressed = false
                    box2_pressed = false
                    box3_pressed = true
                }

                box4_mainText: "SYSTEM"
                box4_subText: "Version"
                onBox4Clicked: {
                    sysStacklayout.currentIndex = 1
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
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/

