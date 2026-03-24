import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    implicitWidth: 280
    implicitHeight: 400

    property bool enclick: true
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
    readonly property var valueslist: ["7", "8", "9", "4", "5", "6", "1", "2", "3", "0", ".", "↵"]

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
                    id: deleteBtn
                    enabled: root.enclick
                    anchors.right: parent.right
                    anchors.rightMargin: 6 * root.scaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    background: Rectangle {
                        implicitWidth: 48 * root.scaleFactor
                        implicitHeight: 32 * root.scaleFactor
                        radius: 8 * root.scaleFactor
                        color: root.colorAccent
                        opacity: 1.0

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
                        text: "⌫"
                        color: root.colorBackground
                        anchors.fill: parent
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
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
                    model: 12
                    delegate: Button {
                        id: btn
                        enabled: root.enclick
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 1

                        //hoverEnabled: true
                        text: root.valueslist[index]
                        required property int index

                        background: Rectangle {
                            color: btn.pressed ? root.colorSurfaceHover : root.colorSurface
                            border.color: btn.pressed ? root.colorAccent : root.colorBorder
                            border.width: 1 * root.scaleFactor
                            radius: 12 * root.scaleFactor

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
                                color: root.valueslist[index]
                                       === "↵" ? Qt.rgba(root.colorAccent.r,
                                                         root.colorAccent.g,
                                                         root.colorAccent.b,
                                                         0.1) : "transparent"
                                visible: valueslist[index] === "↵" ? true : false
                            }
                        }

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: root.valueslist[index]
                                            === "↵" ? 36 : 22 * root.scaleFactor
                            font.bold: root.valueslist[index] !== "↵"
                            color: root.valueslist[index]
                                   === "↵" ? root.colorAccent : root.colorTextPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter

                            font.family: root.valueslist[index] === "0" ? "Segoe UI, Microsoft YaHei, sans-serif" : "Segoe UI, Microsoft YaHei, sans-serif"
                        }

                        onClicked: {
                            if (root.valueslist[index] === "↵"
                                    && defaultField.text.length > 0) {
                                root.entervalue(defaultField.text)
                                defaultField.clear()
                            } else {
                                let val = valueslist[index]
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
