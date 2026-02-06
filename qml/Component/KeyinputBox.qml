import QtQuick 2.15
import QtQuick.Controls 2.15
//import QtQuick.VirtualKeyboard 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: 240
    height: 360
    signal entervalue(real value)

    property real scale: 1.0
    transform: Scale {
        origin.x: 0
        origin.y: 0
        xScale: root.scale
        yScale: root.scale
    }

    Rectangle {
        id: card
        radius: 9
        color: "#0d1b2a"
        anchors.fill: parent

        Column {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 6

            TextField {
                id: defaultField
                width: parent.width
                height: parent.height * 0.18
                placeholderText: "请输入"
                inputMethodHints: Qt.ImhDigitsOnly
                //focus: true // onPressed: {forceActiveFocus()}
                echoMode: TextInput.Normal

                background: Rectangle {
                    radius: 8
                    border.width: 1
                    border.color: "#2196F3" // defaultField.activeFocus ? "#2196F3" : "#ddd"
                    color: "#eee" // defaultField.activeFocus ? "#fff" : "#f9f9f9"
                }

                ToolButton {
                    anchors.right: parent.right
                    anchors.rightMargin: 9
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "" // defaultField.echoMode === TextInput.Password ? "qrc:/icons/visible.svg" : "qrc:/icons/invisible.svg"
                    icon.color: "#666"
                    text: "Enter"
                    font.bold: true
                    visible: true

                    onClicked: {
                        //console.log("dangqianshuzi:" + defaultField.text)
                        root.entervalue(defaultField.text)
                        defaultField.clear()
                        //defaultField.echoMode = defaultField.echoMode=== TextInput.Password ? TextInput.Normal : TextInput.Password
                    }
                }
            }

            Grid {
                id: customNumPad
                columns: 3
                spacing: 5
                width: parent.width
                height: parent.height * 0.8
                anchors.horizontalCenter: parent.horizontalCenter

                Repeater {
                    model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0", ".", "Clear"]
                    Button {
                        width: (customNumPad.width - 10) / 3
                        height: (customNumPad.height - 15) / 4
                        text: modelData
                        font.bold: true
                        onClicked: {
                            if (text === "Clear") {
                                defaultField.clear()
                            } else {
                                defaultField.insert(
                                            defaultField.cursorPosition, text)
                            }
                        }
                    }
                }
            }


            /*InputPanel {
                id: inputPanel
                width: parent.width * 0.5
                height: parent.height * 0.8
                z: 1000 // Level
                visible: true // Qt.inputMethod.visible
                clip: true
                //active: true
            }*/
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:1.1}
}
##^##*/

