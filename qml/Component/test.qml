import QtQuick 2.0
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Item {
    Rectangle {
        width: 200
        height: 200
        radius: 20
        color: "#ffffff"

        Column {
            anchors.centerIn: parent
            spacing: 15

            Rectangle {
                width: 80
                height: 80
                radius: 40
                color: "#2196f3"

                Text {
                    text: "⚙"
                    font.pixelSize: 40
                    color: "white"
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: mainSettings.open()
                }
            }

            Text {
                text: "系统设置"
                font.pixelSize: 18
                font.bold: true
                color: "#333333"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "点击齿轮图标进入设置"
                font.pixelSize: 12
                color: "#666666"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/

