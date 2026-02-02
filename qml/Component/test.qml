import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.VirtualKeyboard 2.15

ApplicationWindow {
    id: mainWindow
    width: 1024
    height: 800
    visible: true

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"

        KeyinputBox {
            anchors.centerIn: parent
            scale: 1
        }
    }
}
