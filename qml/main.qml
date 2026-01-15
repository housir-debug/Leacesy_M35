import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "./Component" // as MyComponents

ApplicationWindow {//ApplicationWindow
    id: mainWindow
    width: 1024
    height: 768
    visible: true
    color: "#0d1b2a"

    // 标题栏
    Rectangle {
        id: titleBar
        width: parent.width
        height: 60
        color: "#1a1a2e"

        Text {
            id: text1
            text: "高精度电源测量系统"
            color: "#ffffff"
            font.pixelSize: 24
            font.bold: true
            anchors.centerIn: parent
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Text {
                text: "●"
                color: "#4cc9f0"
                font.pixelSize: 20
            }

            Text {
                text: "运行中"
                color: "#ffffff"
                font.pixelSize: 14
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // 主内容区
    Rectangle {
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 0
        anchors.bottomMargin: -6
        anchors.leftMargin: 0
        anchors.topMargin: 6
        color: "transparent"

        GridLayout {
            anchors.fill: parent
            anchors.margins: 20
            columns: 2
            columnSpacing: 20
            rowSpacing: 20

            // 左列：主要仪表
            ColumnLayout {
                Layout.column: 0
                Layout.row: 0
                Layout.rowSpan: 2
                spacing: 30

                Gauge {
                    id: voltageGauge
                    Layout.preferredWidth: 300
                    Layout.preferredHeight: 300
                    title: "输出电压"
                    unit: "V"
                    value: 24.5
                    minValue: 0
                    maxValue: 50
                    gaugeColor: "#4cc9f0"
                }

                Gauge {
                    id: currentGauge
                    Layout.preferredWidth: 300
                    Layout.preferredHeight: 300
                    title: "输出电流"
                    unit: "A"
                    value: 2.8
                    minValue: 0
                    maxValue: 10
                    gaugeColor: "#f72585"
                }
            }

            // 右列：数字显示和控制
            ColumnLayout {
                Layout.column: 1
                Layout.row: 0
                spacing: 66

                RowLayout {
                    spacing: 60

                    DigitalDisplay {
                        Layout.preferredWidth: 180
                        label: "功率"
                        unit: "W"
                        value: 68.6
                        textColor: "#ff9e00"
                    }

                    DigitalDisplay {
                        Layout.preferredWidth: 180
                        label: "频率"
                        unit: "Hz"
                        value: 50.00
                        textColor: "#7209b7"
                    }

                    DigitalDisplay {
                        Layout.preferredWidth: 180
                        label: "温度"
                        unit: "°C"
                        value: 35.5
                        textColor: "#ff006e"
                    }
                }

                WaveformDisplay {
                    Layout.preferredHeight: 300
                    Layout.fillWidth: true
                    dataPoints: generateSampleData()
                    waveColor: "#00b4d8"
                }

                // 控制面板
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: 8
                    color: "#1a1a2e"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        anchors.rightMargin: 15
                        anchors.bottomMargin: 15
                        anchors.leftMargin: 15
                        anchors.topMargin: 15

                        Text {
                            text: "控制设置"
                            color: "#ffffff"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        RowLayout {
                            spacing: 20

                            Column {
                                spacing: 5

                                Text {
                                    text: "电压设置"
                                    color: "#cccccc"
                                    font.pixelSize: 12
                                }

                                Slider {
                                    id: voltageSlider
                                    width: 200
                                    from: 0
                                    to: 50
                                    value: 24.5
                                    onValueChanged: voltageGauge.value = value

                                    background: Rectangle {
                                        implicitHeight: 6
                                        color: "#16213e"
                                        radius: 3
                                    }

                                    handle: Rectangle {
                                        width: 20
                                        height: 20
                                        radius: 10
                                        color: "#4cc9f0"
                                        border.color: "#ffffff"
                                    }
                                }
                            }

                            Button {
                                text: "开始测量"
                                font.bold: true
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 40

                                background: Rectangle {
                                    radius: 5
                                    color: parent.down ? "#00509e" :
                                           parent.hovered ? "#0077b6" : "#0096c7"
                                }

                                contentItem: Text {
                                    text: parent.text
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.bold: true
                                }
                            }

                            Button {
                                text: "保存数据"
                                font.bold: true
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 40

                                background: Rectangle {
                                    radius: 5
                                    color: parent.down ? "#5a189a" :
                                           parent.hovered ? "#7b2cbf" : "#9d4edd"
                                }

                                contentItem: Text {
                                    text: parent.text
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function generateSampleData() {
        var data = []
        for (var i = 0; i < 100; i++) {
            data.push(Math.sin(i * 0.1) * 0.8 + Math.random() * 0.2)
        }
        return data
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}
}
##^##*/
