import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.VirtualKeyboard 2.15

ApplicationWindow {
    id: mainWindow
    width: Screen.desktopAvailableWidth
    height: Screen.desktopAvailableHeight
    visibility: "FullScreen"
    visible: true

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"


        /*DigitalCard {
            anchors.centerIn: parent
            scale: 1
        }*/


        /*KeyinputBox {
            anchors.centerIn: parent
            scale: 1
        }*/


        /*SetBox {
            anchors.centerIn: parent
            scale: 1
        }*/


        /*SetBoxGroup {
            anchors.centerIn: parent
            scale: 1
        }*/
        BatteryCard {
            anchors.centerIn: parent
            scale: 1
        }
    }
}
