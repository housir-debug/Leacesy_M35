import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: systemPage

    signal backRequested
    signal changeMode

    property int initialmodel: 0

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
                Layout.preferredWidth: 88
                currentIndex: 0

                // index=0
                KeyinputBox {
                    id: syskeyinput
                    text: Uart_bridge.IPaddress
                    onEntervalue: {
                        //console.log("设置通道状态：" + value)
                        Uart_bridge.update_Configuration(
                                    groupsysbox.currentsetmodel, value)
                    }
                }

                // index=1
                Rectangle {
                    id: versionBox
                    property real scaleFactor: 1.0
                    color: "#0d1b2a"
                    border.color: "#2a3b4c"
                    border.width: 1 * versionBox.scaleFactor
                    radius: 6 * versionBox.scaleFactor

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 2.4 * versionBox.scaleFactor
                        spacing: 2.4 * versionBox.scaleFactor

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 9
                            color: "#2a3b4c"
                            radius: 6 * versionBox.scaleFactor
                            border.color: "#2a3b4c"
                            border.width: 1 * versionBox.scaleFactor

                            RowLayout {
                                anchors.fill: parent

                                Text {
                                    text: "SW-Ver:    V" + Uart_bridge.SoftVer
                                    font.pixelSize: 18 * versionBox.scaleFactor
                                    color: "#e0e0e0"
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                Text {
                                    text: "HW-Ver:    V" + Uart_bridge.HardVer
                                    font.pixelSize: 18 * versionBox.scaleFactor
                                    color: "#e0e0e0"
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }

                        GridLayout {
                            id: channelsGridinfrom
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 91
                            columnSpacing: 1 * versionBox.scaleFactor
                            rowSpacing: 1 * versionBox.scaleFactor
                            columns: 9
                            rows: 4

                            Repeater {
                                model: 36 // channel count
                                delegate: Rectangle {
                                    required property int index
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "#2a3b4c"
                                    radius: 6 * versionBox.scaleFactor
                                    border.color: "#2a3b4c"
                                    border.width: 1 * versionBox.scaleFactor

                                    ColumnLayout {
                                        anchors.fill: parent

                                        Text {
                                            text: "CH_" + (index + 1)
                                            font.pixelSize: 18 * versionBox.scaleFactor
                                            color: "#4a9eff"
                                            Layout.alignment: Qt.AlignHCenter
                                        }

                                        Text {
                                            text: "S:V" + Uart_bridge["ch" + (index + 1) + "_sv"]
                                            font.pixelSize: 12 * versionBox.scaleFactor
                                            color: "#e0e0e0"
                                            Layout.alignment: Qt.AlignHCenter
                                        }

                                        Text {
                                            text: "H:V" + Uart_bridge["ch" + (index + 1) + "_hv"]
                                            font.pixelSize: 12 * versionBox.scaleFactor
                                            color: "#e0e0e0"
                                            Layout.alignment: Qt.AlignHCenter
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SetBoxGroup {
                id: groupsysbox
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 12
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
                }

                box2_mainText: "Other"
                property bool modelsub: false
                box2_subText: modelsub ? "CAN" : "GPIB"
                box2_pressed: false
                onBox2Clicked: {
                    modelsub = !modelsub
                    currentsetmodel = modelsub ? 3 : 2
                    syskeyinput.text = modelsub ? Uart_bridge.CANid : Uart_bridge.GPIBid
                    sysStacklayout.currentIndex = 0
                    box1_pressed = false
                    box2_pressed = true
                }

                box3_mainText: "SYSTEM"
                box3_subText: "Version"
                onBox3Clicked: {
                    sysStacklayout.currentIndex = 1
                }

                box4_mainText: "Model"
                box4_subText: systemPage.initialmodel == 0 ? "General" : "Battery"
                onBox4Clicked: {
                    changeMode()
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

