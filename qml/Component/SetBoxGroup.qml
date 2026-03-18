import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import Component 1.0

Item {
    id: root

    implicitWidth: 88
    implicitHeight: 417

    property bool enclick: true

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }
    property string box1_mainText: "main"
    property string box1_subText: "sub"
    property color box1_mainTextColor: "#E0E0E0"
    property color box1_subTextColor: "#8A9FB0"
    property bool box1_enclick: root.enclick
    property alias box1_pressed: box1.pressed
    signal box1Clicked
    signal box1PressAndHold
    property string box2_mainText: "main"
    property string box2_subText: "sub"
    property color box2_mainTextColor: "#E0E0E0"
    property color box2_subTextColor: "#8A9FB0"
    property bool box2_enclick: root.enclick
    property alias box2_pressed: box2.pressed
    signal box2Clicked
    signal box2PressAndHold
    property string box3_mainText: "main"
    property string box3_subText: "sub"
    property color box3_mainTextColor: "#E0E0E0"
    property color box3_subTextColor: "#8A9FB0"
    property bool box3_enclick: root.enclick
    property alias box3_pressed: box3.pressed
    signal box3Clicked
    signal box3PressAndHold
    property string box4_mainText: "main"
    property string box4_subText: "sub"
    property color box4_mainTextColor: "#E0E0E0"
    property color box4_subTextColor: "#8A9FB0"
    property bool box4_enclick: root.enclick
    property alias box4_pressed: box4.pressed
    signal box4Clicked
    signal box4PressAndHold
    property string box5_mainText: "main"
    property string box5_subText: "sub"
    property color box5_mainTextColor: "#E0E0E0"
    property color box5_subTextColor: "#8A9FB0"
    property bool box5_enclick: root.enclick
    property alias box5_pressed: box5.pressed
    signal box5Clicked
    signal box5PressAndHold

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 12 * root.scaleFactor
        color: "#0A1929"

        Rectangle {
            anchors.fill: parent
            radius: parent.radius - 1
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(255, 255, 255, 0.05)
        }

        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop {
                position: 0.0
                color: Qt.lighter("#0A1929", 1.2)
            }
            GradientStop {
                position: 0.5
                color: "#0A1929"
            }
            GradientStop {
                position: 1.0
                color: Qt.darker("#0A1929", 1.1)
            }
        }

        layer {
            enabled: true
            effect: DropShadow {
                transparentBorder: true
                color: Qt.rgba(0, 0, 0, 0.3)
                radius: 16 * root.scaleFactor
                samples: 20
                horizontalOffset: 0
                verticalOffset: 2
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 3.6 * root.scaleFactor
        spacing: 3.6 * root.scaleFactor
        z: 1

        SetBox {
            id: box1
            Layout.fillHeight: true
            Layout.fillWidth: true

            scaleFactor: root.scaleFactor
            mainText: root.box1_mainText
            subText: root.box1_subText
            mainTextColor: root.box1_mainTextColor
            subTextColor: root.box1_subTextColor
            enclick: root.box1_enclick

            onClicked: root.box1Clicked()
            onPressAndHold: root.box1PressAndHold()
        }

        SetBox {
            id: box2
            Layout.fillHeight: true
            Layout.fillWidth: true

            scaleFactor: root.scaleFactor
            mainText: root.box2_mainText
            subText: root.box2_subText
            mainTextColor: root.box2_mainTextColor
            subTextColor: root.box2_subTextColor
            enclick: root.box2_enclick

            onClicked: root.box2Clicked()
            onPressAndHold: root.box2PressAndHold()
        }

        SetBox {
            id: box3
            Layout.fillHeight: true
            Layout.fillWidth: true

            scaleFactor: root.scaleFactor
            mainText: root.box3_mainText
            subText: root.box3_subText
            mainTextColor: root.box3_mainTextColor
            subTextColor: root.box3_subTextColor
            enclick: root.box3_enclick

            onClicked: root.box3Clicked()
            onPressAndHold: root.box3PressAndHold()
        }

        SetBox {
            id: box4
            Layout.fillHeight: true
            Layout.fillWidth: true

            scaleFactor: root.scaleFactor
            mainText: root.box4_mainText
            subText: root.box4_subText
            mainTextColor: root.box4_mainTextColor
            subTextColor: root.box4_subTextColor
            enclick: root.box4_enclick

            onClicked: root.box4Clicked()
            onPressAndHold: root.box4PressAndHold()
        }

        SetBox {
            id: box5
            Layout.fillHeight: true
            Layout.fillWidth: true

            scaleFactor: root.scaleFactor
            mainText: root.box5_mainText
            subText: root.box5_subText
            mainTextColor: root.box5_mainTextColor
            subTextColor: root.box5_subTextColor
            enclick: root.box5_enclick

            onClicked: root.box5Clicked()
            onPressAndHold: root.box5PressAndHold()
        }
    }
}
