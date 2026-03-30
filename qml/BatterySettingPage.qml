// Setting.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: batterysettingPage

    signal backRequested

    property int initialChannel: 0

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 3

            BatteryBar {
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
                voltageUnit: "V"
                current: settingPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + settingPage.initialChannel + "_Current"] : 0.0
                currentUnit: settingPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + settingPage.initialChannel
                                                 + "_CurrentUnit"] : "A"
                esr: settingPage.initialChannel
                     !== 0 ? Uart_bridge["ch" + (index + 1) + "_imp"] : 0.0
                esrUnit: "Ω"

                batteryMode: settingPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + (index + 1) + "_BatteryMode"] : "model-1"
                workMode: settingPage.initialChannel
                          !== 0 ? Uart_bridge["ch" + (index + 1) + "_WorkMode"] : "static"
                batteryCapacity: settingPage.initialChannel
                                 !== 0 ? Uart_bridge["ch" + (index + 1) + "_CapacityAH"] : 0.0

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

            StackLayout {
                id: setStacklayout
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 88
                currentIndex: 0

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

                Tumbler {
                    id: tumbler
                    anchors.centerIn: parent
                    model: ["选项 A", "选项 B", "选项 C", "选项 D", "选项 E"]

                    // 可选：自定义每一项的显示样式
                    delegate: Text {
                        text: modelData
                        opacity: 0.6 + Math.max(0, 1 - Math.abs(
                                                    Tumbler.displacement)) * 0.4
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    // 关键信号：currentIndex 改变时触发
                    onCurrentIndexChanged: {
                        console.log("选中了:", model[currentIndex])
                        // 在这里执行你的更新逻辑
                        updateContent(model[currentIndex])
                    }
                }

                function updateContent(selected) {
                    // 你的业务逻辑
                    console.log("执行更新:", selected)
                }
            }

            SetBoxGroup {
                id: groupsetbox
                Layout.fillHeight: true
                Layout.fillWidth: true
                enclick: !Uart_bridge.isRemote
                property int currentsetmodel: 0

                box1_mainText: "SOC"
                box1_subText: "init"
                box1_pressed: true
                onBox1Clicked: {
                    currentsetmodel = 0
                    setStacklayout.currentIndex = 0
                    box1_pressed = true
                    box2_pressed = false
                    box3_pressed = false
                }

                box2_mainText: "Battery"
                box2_subText: "model"
                box2_pressed: false
                onBox2Clicked: {
                    currentsetmodel = 1
                    setStacklayout.currentIndex = 1
                    box1_pressed = false
                    box2_pressed = true
                    box3_pressed = false
                }

                box3_mainText: "model"
                box3_subText: "static"
                box3_pressed: false
                onBox3Clicked: {
                    setStacklayout.currentIndex = 1
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

