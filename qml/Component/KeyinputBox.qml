import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    implicitWidth: 280
    implicitHeight: 400
    width: implicitWidth * scaleFactor
    height: implicitHeight * scaleFactor

    property real scaleFactor: 1.0
    property alias text: defaultField.text

    signal entervalue(real value)

    readonly property color colorBackground: "#0A1929"
    readonly property color colorSurface: "#102B40"
    readonly property color colorSurfaceHover: "#1E3A5F"
    readonly property color colorBorder: "#1E3A5F"
    readonly property color colorBorderLight: "#2A4A70"
    readonly property color colorAccent: "#4A9EFF"
    readonly property color colorAccentGlow: "#3D8CFF"
    readonly property color colorTextPrimary: "#E0E0E0"
    readonly property color colorTextSecondary: "#8A9FB0"
    readonly property color colorPlaceholder: "#5F7D9C"

    Rectangle {
        id: card
        radius: 16 * scaleFactor
        color: root.colorBackground
        anchors.fill: parent
        border.width: 1
        border.color: Qt.rgba(root.colorAccent.r, root.colorAccent.g,
                              root.colorAccent.b, 0.2)

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(root.colorAccent.r, root.colorAccent.g,
                                  root.colorAccent.b, 0.1)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12 * scaleFactor
            spacing: 8 * scaleFactor

            TextField {
                id: defaultField
                Layout.fillWidth: true
                Layout.preferredHeight: 56 * scaleFactor

                placeholderText: "请输入"
                placeholderTextColor: root.colorPlaceholder
                inputMethodHints: Qt.ImhDigitsOnly
                echoMode: TextInput.Normal
                font.pixelSize: 18 * scaleFactor
                font.family: "Segoe UI, Microsoft YaHei, sans-serif"
                color: root.colorTextPrimary
                selectionColor: root.colorAccent
                selectedTextColor: root.colorBackground

                background: Rectangle {
                    radius: 12 * scaleFactor
                    color: root.colorSurface
                    border.width: defaultField.activeFocus ? 2 : 1
                    border.color: defaultField.activeFocus ? root.colorAccent : root.colorBorder

                    layer {
                        enabled: defaultField.activeFocus
                        effect: DropShadow {
                            transparentBorder: true
                            color: Qt.rgba(root.colorAccent.r,
                                           root.colorAccent.g,
                                           root.colorAccent.b, 0.3)
                            radius: 12
                            samples: 24
                            horizontalOffset: 0
                            verticalOffset: 0
                        }
                    }
                }

                ToolButton {
                    id: enterBtn
                    anchors.right: parent.right
                    anchors.rightMargin: 6 * scaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    background: Rectangle {
                        implicitWidth: 48 * scaleFactor
                        implicitHeight: 32 * scaleFactor
                        radius: 8 * scaleFactor
                        color: root.colorAccent
                        opacity: enterBtn.hovered ? 1.0 : 0.9

                        Behavior on opacity {
                            NumberAnimation {
                                duration: 150
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: parent.height / 2
                            radius: parent.radius
                            gradient: Gradient {
                                GradientStop {
                                    position: 0.0
                                    color: Qt.rgba(1, 1, 1, 0.2)
                                }
                                GradientStop {
                                    position: 1.0
                                    color: Qt.rgba(1, 1, 1, 0)
                                }
                            }
                        }
                    }

                    contentItem: Text {
                        text: "↵"
                        font.pixelSize: 22 * scaleFactor
                        font.bold: true
                        color: root.colorBackground
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        let num = parseFloat(defaultField.text)
                        if (!isNaN(num)) {
                            root.entervalue(num)
                            defaultField.clear()
                        } else {
                            if (defaultField.text.length > 0) {
                                defaultField.clear()
                            }
                        }
                    }
                }

                Rectangle {
                    width: 2
                    height: 24 * scaleFactor
                    color: root.colorAccent
                    opacity: 0.5
                    anchors.left: parent.left
                    anchors.leftMargin: 4 * scaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            GridLayout {
                id: keypadGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 3
                columnSpacing: 8 * scaleFactor
                rowSpacing: 8 * scaleFactor

                Repeater {
                    model: [{
                            "label": "7",
                            "value": "7"
                        }, {
                            "label": "8",
                            "value": "8"
                        }, {
                            "label": "9",
                            "value": "9"
                        }, {
                            "label": "4",
                            "value": "4"
                        }, {
                            "label": "5",
                            "value": "5"
                        }, {
                            "label": "6",
                            "value": "6"
                        }, {
                            "label": "1",
                            "value": "1"
                        }, {
                            "label": "2",
                            "value": "2"
                        }, {
                            "label": "3",
                            "value": "3"
                        }, {
                            "label": "0",
                            "value": "0"
                        }, {
                            "label": ".",
                            "value": "."
                        }, {
                            "label": "⌫",
                            "isClear": true
                        }]

                    delegate: Button {
                        id: btn
                        Layout.preferredWidth: 1
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.columnSpan: 1

                        text: modelData.label

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: parent.clicked()
                            cursorShape: Qt.PointingHandCursor
                            pressAndHoldInterval: 500
                            onPressAndHold: {
                                if (modelData.isClear) {
                                    defaultField.clear()
                                }
                            }
                        }

                        background: Rectangle {
                            color: mouseArea.containsMouse ? root.colorSurfaceHover : root.colorSurface
                            radius: 12 * scaleFactor
                            border.width: 1
                            border.color: mouseArea.containsMouse ? root.colorAccent : root.colorBorder

                            Behavior on color {
                                ColorAnimation {
                                    duration: 150
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: parent.height / 2
                                radius: parent.radius
                                color: "transparent"
                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: Qt.rgba(1, 1, 1, 0.1)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: Qt.rgba(1, 1, 1, 0)
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius
                                color: "transparent"
                                border.width: mouseArea.pressed ? 2 : 0
                                border.color: Qt.rgba(root.colorAccent.r,
                                                      root.colorAccent.g,
                                                      root.colorAccent.b, 0.5)
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius
                                color: modelData.isClear ? Qt.rgba(
                                                               root.colorAccent.r,
                                                               root.colorAccent.g,
                                                               root.colorAccent.b,
                                                               0.1) : "transparent"
                                visible: modelData.isClear
                            }
                        }

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: modelData.isClear ? 24 : 22 * scaleFactor
                            font.bold: !modelData.isClear
                            color: modelData.isClear ? root.colorAccent : root.colorTextPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter

                            font.family: modelData.label === "0" ? "Segoe UI, Microsoft YaHei, sans-serif" : "Segoe UI, Microsoft YaHei, sans-serif"
                        }

                        onClicked: {
                            if (modelData.isClear) {
                                defaultField.remove(
                                            defaultField.cursorPosition - 1,
                                            defaultField.cursorPosition)
                            } else {
                                let val = modelData.value
                                defaultField.insert(
                                            defaultField.cursorPosition, val)
                            }
                        }
                    }
                }
            }
        }
    }

    Behavior on scaleFactor {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutQuad
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:1.1}
}
##^##*/

