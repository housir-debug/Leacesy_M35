import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: systemPage

    signal backRequested

    property int initialChannel: 0
    property int currentsetmodel: 0

    Rectangle {
        anchors.fill: parent
        color: "#0d1b2a"
    }
}
