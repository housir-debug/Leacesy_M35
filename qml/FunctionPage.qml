// Function.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: functionPage

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
                Layout.preferredWidth: 360
                enclick: !Uart_bridge.isRemote
                channelOutput: functionPage.initialChannel
                               !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                   + "_isOutput"] : false
                channelName: functionPage.initialChannel
                             === 0 ? "CH*" : "CH" + functionPage.initialChannel
                voltage: functionPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Voltage"] : 0.0
                current: functionPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Current"] : 0.0
                voltageUnit: "V"
                currentUnit: functionPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                 + "_CurrentUnit"] : "A"
                cvSetpoint: functionPage.initialChannel
                            !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_cv"] : 0.0
                ccSetpoint: functionPage.initialChannel
                            !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_cc"] : 1.0
                ovpSetpoint: functionPage.initialChannel
                             !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_ovp"] : 8.0
                cvModel: functionPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Status"].charAt(
                                     14) === "1" : false
                ccModel: functionPage.initialChannel
                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Status"].charAt(
                                     13) === "1" : false
                ovpModel: functionPage.initialChannel
                          !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                              + "_Status"].charAt(
                                      11) === "1" : false

                onClicked: {
                    Uart_bridge.setChannel_Output(functionPage.initialChannel,
                                                  !channelOutput)
                    if (functionPage.initialChannel === 0) {
                        channelOutput = !channelOutput
                    }
                }


                /*onPressAndHold: {
                    if (channelOutput) {
                    } else {
                    }---用于扩展功能
                }*/
            }

            Rectangle {
                id: statusIndicator
                property real scaleFactor: 1.0
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 360
                color: "#0d1b2a"
                border.color: "#1a2f42"
                border.width: 1 * statusIndicator.scaleFactor
                radius: 18 * statusIndicator.scaleFactor

                readonly property color colorGlow: "#1E5799"
                readonly property var statusLabels: ["OPC", "WAI", "LCC", "ERR", "CP", "FAN", "CAL", "RL", "IMP", "OT", "OC", "OVP", "LD", "CC", "CV", "PON"]

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 8 * statusIndicator.scaleFactor
                    rowSpacing: 3 * statusIndicator.scaleFactor
                    columnSpacing: 3 * statusIndicator.scaleFactor
                    columns: 4
                    rows: 4

                    Repeater {
                        model: 16
                        delegate: Rectangle {
                            id: indicator
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            color: isActive ? "#0F2847" : "#0A1929"
                            border.color: isActive ? "#1E5799" : "#1E3A5F"
                            border.width: 1.8 * statusIndicator.scaleFactor
                            radius: 6 * statusIndicator.scaleFactor

                            required property int index
                            property bool isActive: functionPage.initialChannel
                                                    !== 0 ? Uart_bridge["ch" + functionPage.initialChannel + "_Status"].charAt(
                                                                index) === "1" : false
                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: parent.height / 2
                                radius: parent.radius

                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: Qt.rgba(
                                                   1, 1, 1,
                                                   indicator.isActive ? 0.2 : 0.08)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: "transparent"
                                    }
                                }
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: parent.height / 2
                                radius: parent.radius

                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: "transparent"
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: Qt.rgba(0, 0, 0, 0.3)
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius - 1.5
                                color: "transparent"
                                border.width: 1.0
                                border.color: Qt.rgba(
                                                  statusIndicator.colorGlow.r,
                                                  statusIndicator.colorGlow.g,
                                                  statusIndicator.colorGlow.b,
                                                  indicator.isActive ? 0.3 : 0.1)
                            }

                            Text {
                                anchors.centerIn: parent
                                text: statusIndicator.statusLabels[index]
                                color: indicator.isActive ? "#CCCCCC" : "#6a9aaf"
                                font.pixelSize: 18 * statusIndicator.scaleFactor
                                font.bold: true
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
                property int currentsetmodel: 0

                box1_mainText: ""
                box1_subText: ""
                box1_pressed: false
                box1_enclick: false
                onBox1Clicked: {

                }

                box2_mainText: ""
                box2_subText: ""
                box2_pressed: false
                box2_enclick: false
                onBox2Clicked: {

                }

                box3_mainText: ""
                box3_subText: ""
                box3_pressed: false
                box3_enclick: false
                onBox3Clicked: {

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

