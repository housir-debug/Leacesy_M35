import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: mainWindow
    //width: Screen.desktopAvailableWidth
    //height: Screen.desktopAvailableHeight
    width: 1440
    height: 400
    visibility: "FullScreen"
    visible: true

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        Rectangle {
            width: 9
            height: 9
            radius: height / 2
            //color: "#0000FF"
            anchors.centerIn: parent
        }


        /*DigitalCard {
            anchors.centerIn: parent
            scale: 1
        }*/


        /*BatteryCard {
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


        /*Unlock {
            anchors.centerIn: parent
            scale: 1
        }*/
    }
}
