import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    //ApplicationWindow
    id: mainWindow
    width: 1024
    height: 800
    visible: true

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        Row {
            anchors.centerIn: parent
            anchors.margins: 10
            spacing: 150

            DigitalCard {
                id: channel_1
                channelName: "CH1"
                scale: 1.6
            }

            DigitalCard {
                id: channel_2
                channelName: "CH2"
                scale: 1.6
            }

            Column {
                anchors.margins: 0
                spacing: 15

                SetBox {
                    id: system_box
                    scale: 1.1
                }

                SetBox {
                    id: setting_box
                    scale: 1.1
                }

                SetBox {
                    id: switch_box
                    scale: 1.1
                }

                SetBox {
                    id: unit_box
                    scale: 1.1
                }

                SetBox {
                    id: model_box
                    scale: 1.1
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}
}
##^##*/

