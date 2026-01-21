import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Item {
    RoundButton {
        id: settingsBtn
        text: "⚙"
        width: 40
        height: 40
        radius: 6

        onClicked: settingsPopup.open()

        Popup {
            id: settingsPopup
            x: settingsBtn.width
            y: 0
            width: 200
            height: contentColumn.implicitHeight + 20

            background: Rectangle {
                color: "#ffffff"
                radius: 8
                border.color: "#cccccc"
                layer.enabled: true
                layer.effect: DropShadow {
                    radius: 8
                    samples: 17
                    color: "#40000000"
                }
            }

            Column {
                id: contentColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Text {
                    text: "设置菜单"
                    font.bold: true
                    font.pixelSize: 14
                    color: "#333333"
                }

                Repeater {
                    model: ["显示设置", "声音设置", "网络设置", "高级设置"]

                    Button {
                        width: parent.width
                        text: modelData
                        flat: true
                        onClicked: {
                            console.log("选择:", modelData)
                            settingsPopup.close()
                        }
                    }
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

