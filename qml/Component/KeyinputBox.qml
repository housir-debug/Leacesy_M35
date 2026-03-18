import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    implicitWidth: 280
    implicitHeight: 400

    property alias text: defaultField.text
    signal entervalue(string value)

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }
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
        anchors.fill: parent
        radius: 16 * root.scaleFactor
        color: root.colorBackground
        border.width: 1 * root.scaleFactor
        border.color: Qt.rgba(root.colorAccent.r, root.colorAccent.g,
                              root.colorAccent.b, 0.2)

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 2 * root.scaleFactor
            border.color: Qt.rgba(root.colorAccent.r, root.colorAccent.g,
                                  root.colorAccent.b, 0.1)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12 * root.scaleFactor
            spacing: 8 * root.scaleFactor

            TextField {
                id: defaultField
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 16

                placeholderText: "请输入"
                placeholderTextColor: root.colorPlaceholder
                inputMethodHints: Qt.ImhNone //Qt.ImhDigitsOnly
                echoMode: TextInput.Normal
                font.pixelSize: 18 * root.scaleFactor
                font.family: "Segoe UI, Microsoft YaHei, sans-serif"
                color: root.colorTextPrimary
                selectionColor: root.colorAccent
                selectedTextColor: root.colorBackground

                background: Rectangle {
                    radius: 12 * root.scaleFactor
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
                            radius: 12 * root.scaleFactor
                            samples: 24
                            horizontalOffset: 0
                            verticalOffset: 0
                        }
                    }
                }

                ToolButton {
                    id: enterBtn
                    anchors.right: parent.right
                    anchors.rightMargin: 6 * root.scaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    background: Rectangle {
                        implicitWidth: 48 * root.scaleFactor
                        implicitHeight: 32 * root.scaleFactor
                        radius: 8 * root.scaleFactor
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
                        font.pixelSize: 22 * root.scaleFactor
                        font.bold: true
                        color: root.colorBackground
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (defaultField.text.length > 0) {
                            root.entervalue(defaultField.text)
                            defaultField.clear()
                        }
                    }
                }

                Rectangle {
                    width: 2 * root.scaleFactor
                    height: 24 * root.scaleFactor
                    color: root.colorAccent
                    opacity: 0.5
                    anchors.left: parent.left
                    anchors.leftMargin: 4 * root.scaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 84
                columnSpacing: 8 * root.scaleFactor
                rowSpacing: 8 * root.scaleFactor
                columns: 3
                rows: 4

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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 1

                        text: modelData.label
                        hoverEnabled: true

                        background: Rectangle {
                            color: btn.hovered ? root.colorSurfaceHover : root.colorSurface
                            radius: 12 * root.scaleFactor
                            border.width: 1
                            border.color: btn.hovered ? root.colorAccent : root.colorBorder

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
                                border.width: btn.pressed ? 2 : 0
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
                                visible: modelData.isClear ? true : false
                            }
                        }

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: modelData.isClear ? 24 : 22 * root.scaleFactor
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
