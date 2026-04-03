// Function.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: functionPage

    signal backRequested

    property color backgroundcolor: "#0d1b2a" //"#0a0f1a"
    property int initialChannel: 0
    property int scalefactor: 1.0

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1.8 * functionPage.scalefactor

            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 66
                color: backgroundcolor

                ColumnLayout {
                    anchors.fill: parent

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "Software-Ver:"
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge.SoftVer
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "Hardware-Ver:"
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge.HardVer
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "CH-Software-Ver:"
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge["ch" + (index + 1) + "_sv"]
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: backgroundcolor

                        Text {
                            text: "CH-Hardware-Ver:"
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "#1E3A5F"

                        Text {
                            text: "V: " + Uart_bridge["ch" + (index + 1) + "_hv"]
                            font.pixelSize: 18 * versionBox.scaleFactor
                            color: "#e0e0e0"
                            anchors.centerIn: parent
                        }
                    }
                }
            }

            Rectangle {
                id: statusIndicator
                property real scaleFactor: 1.0
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 66
                color: backgroundcolor
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

            StackLayout {
                id: stackLayout
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 132
                currentIndex: 0

                Rectangle {
                    anchors.fill: parent
                    color: backgroundcolor

                    RowLayout {
                        anchors.fill: parent

                        DigitalCard {
                            id: channel
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.preferredWidth: 66
                            enclick: !Uart_bridge.isRemote
                            channelOutput: functionPage.initialChannel
                                           !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                               + "_isOutput"] : false
                            channelName: functionPage.initialChannel
                                         === 0 ? "CH*" : "CH" + functionPage.initialChannel
                            voltage: functionPage.initialChannel
                                     !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                         + "_Voltage"] : 0.0
                            current: functionPage.initialChannel
                                     !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                         + "_Current"] : 0.0
                            voltageUnit: "V"
                            currentUnit: functionPage.initialChannel
                                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                             + "_CurrentUnit"] : "A"
                            cvSetpoint: functionPage.initialChannel
                                        !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                            + "_cv"] : 0.0
                            ccSetpoint: functionPage.initialChannel
                                        !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                            + "_cc"] : 1.0
                            ovpSetpoint: functionPage.initialChannel
                                         !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                             + "_ovp"] : 8.0
                            cvMode: functionPage.initialChannel
                                    !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                        + "_Status"].charAt(
                                                14) === "1" : false
                            ccMode: functionPage.initialChannel
                                    !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                        + "_Status"].charAt(
                                                13) === "1" : false
                            ovpMode: functionPage.initialChannel
                                     !== 0 ? Uart_bridge["ch" + functionPage.initialChannel
                                                         + "_Status"].charAt(
                                                 11) === "1" : false

                            onClicked: {
                                Uart_bridge.setChannel_Output(
                                            functionPage.initialChannel,
                                            !channelOutput)
                                if (functionPage.initialChannel === 0) {
                                    channelOutput = !channelOutput
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.preferredWidth: 66

                            SetBox {
                                id: cv
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                mainText: "CV"
                                subText: "0.000 V"
                                pressed: settingPage.currentsetmodel === 9

                                enclick: !Uart_bridge.isRemote
                                onClicked: {
                                    settingPage.currentsetmodel = 9
                                    keyinput.text = subText
                                }
                            }
                            SetBox {
                                id: cc
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                mainText: "CC"
                                subText: "1.000 A"
                                pressed: settingPage.currentsetmodel === 10

                                enclick: !Uart_bridge.isRemote
                                onClicked: {
                                    settingPage.currentsetmodel = 10
                                    keyinput.text = subText
                                }
                            }
                            SetBox {
                                id: ovp
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                mainText: "OVP"
                                subText: "8.000 V"
                                pressed: settingPage.currentsetmodel === 11

                                enclick: !Uart_bridge.isRemote
                                onClicked: {
                                    settingPage.currentsetmodel = 11
                                    keyinput.text = subText
                                }
                            }
                            SetBox {
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                mainText: "I-Unit"
                                subText: "A"
                                subTextColor: "#9A6AB0"

                                enclick: !Uart_bridge.isRemote
                                onClicked: {
                                    subText = Uart_bridge.setChannel_CurrentUnit(
                                                0)
                                }
                            }
                        }
                    }
                }
            }

            KeyinputBox {
                id: syskeyinput
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 66
                onEntervalue: {
                    //console.log("设置通道状态：" + value)
                    Uart_bridge.update_Configuration(
                                groupsysbox.currentsetmodel, value)
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.66;height:360;width:960}
}
##^##*/

