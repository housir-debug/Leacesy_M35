import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    property bool enclick: true
    property alias text: defaultField.text

    signal entervalue(string value)

    implicitWidth: 280
    implicitHeight: 400

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }

    Rectangle {
        color: "#0A1929"
        anchors.fill: parent
        radius: 18 * root.scaleFactor
        border.width: 1.8 * root.scaleFactor
        border.color: "#004095"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 9 * root.scaleFactor
            spacing: 9 * root.scaleFactor

            TextField {
                id: defaultField
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 16

                placeholderText: "请输入"
                placeholderTextColor: "#5F7D9C"
                font.pixelSize: 18 * root.scaleFactor
                inputMethodHints: Qt.ImhNone //Qt.ImhDigitsOnly
                echoMode: TextInput.Normal
                focus: false

                color: "#E0E0E0"
                selectionColor: "#1A6ECF"
                selectedTextColor: "#0A1929"

                background: Rectangle {
                    radius: 9 * root.scaleFactor
                    color: "#102B40"
                    border.width: defaultField.text.length > 0 ? 1.8 : 0.9
                    border.color: defaultField.text.length > 0 ? "#1A6ECF" : "#1E3A5F"

                    layer {
                        enabled: defaultField.text.length > 0
                        effect: DropShadow {
                            transparentBorder: true
                            color: Qt.rgba(0.24, 0.63, 0.90, 0.3)
                            radius: 9 * root.scaleFactor
                            horizontalOffset: 0
                            verticalOffset: 0
                            samples: 24
                        }
                    }
                }

                ToolButton {
                    id: deleteBtn
                    enabled: root.enclick
                    anchors.right: parent.right
                    anchors.rightMargin: 9 * root.scaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    height: 36 * root.scaleFactor
                    width: 45 * root.scaleFactor

                    contentItem: Text {
                        text: "⌫"
                        color: "#0A1929"
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }

                    background: Rectangle {
                        radius: 9 * root.scaleFactor
                        color: "#1A6ECF"

                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            gradient: Gradient {
                                GradientStop {
                                    position: 0.0
                                    color: Qt.rgba(1, 1, 1, 0.18)
                                }
                                GradientStop {
                                    position: 1.0
                                    color: Qt.rgba(0.36, 0.36, 0.36, 0.81)
                                }
                            }
                        }
                    }

                    onClicked: {
                        defaultField.remove(defaultField.cursorPosition - 1,
                                            defaultField.cursorPosition)
                    }

                    onPressAndHold: {
                        defaultField.clear()
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.leftMargin: 3.6 * root.scaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1.8 * root.scaleFactor
                    height: parent.height * 0.36
                    color: "#1A6ECF"
                }
            }

            GridLayout {
                id: grid
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 84
                columnSpacing: 9 * root.scaleFactor
                rowSpacing: 9 * root.scaleFactor
                columns: 3
                rows: 4

                readonly property var valueslist: ["7", "8", "9", "4", "5", "6", "1", "2", "3", "0", ".", "↵"]

                Repeater {
                    model: 12
                    delegate: Button {
                        id: btn
                        enabled: root.enclick
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 1

                        //hoverEnabled: true
                        required property int index

                        background: Rectangle {
                            color: btn.pressed ? "#1E3A5F" : "#102B40"
                            border.color: btn.pressed ? "#1A6ECF" : "#1E3A5F"
                            border.width: 1 * root.scaleFactor
                            radius: 18 * root.scaleFactor

                            layer.enabled: text === "↵"
                            layer.effect: DropShadow {
                                transparentBorder: true
                                color: Qt.rgba(0.24, 0.63, 0.90, 0.81)
                                radius: 9 * root.scaleFactor
                                samples: 16
                            }
                        }

                        contentItem: Text {
                            font.bold: true
                            font.pixelSize: 27 * root.scaleFactor
                            color: grid.valueslist[index] === "↵" ? "#1A6ECF" : "#E0E0E0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: grid.valueslist[index]
                        }

                        onClicked: {
                            if (grid.valueslist[index] === "↵"
                                    && defaultField.text.length > 0) {
                                root.entervalue(defaultField.text)
                                defaultField.clear()
                            } else {
                                let val = grid.valueslist[index]
                                defaultField.insert(
                                            defaultField.cursorPosition, val)
                            }
                        }
                    }
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:400;width:280}
}
##^##*/

