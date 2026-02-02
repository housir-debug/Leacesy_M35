import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.VirtualKeyboard 2.15

Item {
    id: root
    width: 860
    height: 360

    signal valueChanged(real value)

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
            anchors.margins: 8
            spacing: 6

            TextField {
                id: defaultField
                width: parent.width
                height: parent.height * 0.18
                placeholderText: "请输入"
                //horizontalAlignment: TextField.AlignHCenter
                inputMethodHints: Qt.ImhDigitsOnly
                echoMode: TextInput.Normal

                background: Rectangle {
                    radius: 8
                    border.width: 1
                    border.color: defaultField.activeFocus ? "#2196F3" : "#ddd"
                    color: defaultField.activeFocus ? "#fff" : "#f9f9f9"
                }

                ToolButton {
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    //icon.source: defaultField.echoMode === TextInput.Password ? "qrc:/icons/visible.svg" : "qrc:/icons/invisible.svg"
                    icon.color: "#999"
                    visible: false
                    onClicked: {

                        //defaultField.echoMode = defaultField.echoMode=== TextInput.Password ? TextInput.Normal : TextInput.Password
                    }
                }

                focus: true // onPressed: {forceActiveFocus()}
                onAccepted: {
                    root.valueChanged(Number(text))
                }
            }

            InputPanel {
                id: inputPanel
                width: parent.width
                height: parent.height * 0.8
                z: 1000 // Level
                visible: true // Qt.inputMethod.visible
                //active: true
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:1.1}
}
##^##*/

