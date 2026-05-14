import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property bool enclick: true
    property bool channelOutput: false

    property string channelName: "CH1"
    property real voltage: 0.0
    property string voltageUnit: "V"
    property real current: 0.0
    property string currentUnit: "A"

    property real cvSetpoint: 0.0
    property bool cvMode: false
    property real ccSetpoint: 1.0
    property bool ccMode: false
    property real ovpSetpoint: 8.0
    property bool ovpMode: false

    signal clicked
    signal pressAndHold

    implicitWidth: 280
    implicitHeight: 400

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }
    property bool pressed: false
    property bool longpressed: false

    Rectangle {
        id: card
        anchors.fill: parent
        radius: 36 * root.scaleFactor
        border.width: 1.8 * root.scaleFactor
        border.color: root.channelOutput ? "#4A6FA5" : "#2B2B3C"
        color: root.channelOutput ? "#363648" : "#181824"

        Rectangle {
            width: parent.width * 0.7
            height: 0.9 * root.scaleFactor
            color: root.channelOutput ? "#6B8ABC" : "#404050"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 8.1 * root.scaleFactor
            anchors.top: parent.top
        }

        Rectangle {
            width: parent.width * 0.7
            height: 0.9 * root.scaleFactor
            color: root.channelOutput ? "#6B8ABC" : "#404050"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 8.1 * root.scaleFactor
            anchors.bottom: parent.bottom
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18 * root.scaleFactor
            spacing: 18 * root.scaleFactor

            Item {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: parent.height * 0.18

                Text {
                    id: channelText
                    color: "#E0E0E0"
                    font.bold: true
                    font.pixelSize: 36 * root.scaleFactor
                    anchors.centerIn: parent
                    text: root.channelName
                    //opacity: 0.81
                }

                Rectangle {
                    id: channelDot
                    anchors {
                        verticalCenter: parent.verticalCenter
                        rightMargin: 6 * root.scaleFactor
                        right: parent.right
                    }

                    radius: width / 2
                    width: 36 * root.scaleFactor
                    height: 36 * root.scaleFactor
                    color: root.channelOutput ? "#1AF080" : "#6A6A7E"
                    border.width: 1.2 * root.scaleFactor
                    border.color: "#363636"
                }
            }

            Rectangle {
                id: measurementCard
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * 0.4

                color: "#080810"
                border.color: "#3A3A4E"
                border.width: 1.2 * root.scaleFactor
                radius: 18 * root.scaleFactor

                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop {
                        position: 0.0
                        color: "#181824"
                    }
                    GradientStop {
                        position: 1.0
                        color: "#090918"
                    }
                }

                Rectangle {
                    color: "#404050"
                    width: parent.width * 0.81
                    height: 0.9 * root.scaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottomMargin: 6.6 * root.scaleFactor
                    anchors.bottom: parent.bottom
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12 * root.scaleFactor

                    Text {
                        id: voltageText
                        color: "#0DAF65"
                        Layout.alignment: Qt.AlignHCenter
                        text: root.voltage.toFixed(4) + " " + root.voltageUnit
                        font.pixelSize: 36 * root.scaleFactor
                        font.bold: true
                    }

                    Row {
                        spacing: 6 * root.scaleFactor
                        Layout.alignment: Qt.AlignHCenter

                        Text {
                            id: currentText
                            color: "#DF1D32"
                            text: root.current.toFixed(4)
                            font.pixelSize: 36 * root.scaleFactor
                            font.bold: true
                        }

                        Text {
                            id: unitText
                            color: "#DF1D32"
                            text: root.currentUnit
                            anchors.baseline: currentText.baseline
                            font.pixelSize: (root.currentUnit == "mA" ? 18 : 36) * root.scaleFactor
                            font.bold: true
                        }
                    }
                }
            }

            ColumnLayout {
                id: setpointsColumn
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * 0.3
                spacing: 6 * root.scaleFactor

                Item {
                    id: cvRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30 * root.scaleFactor

                    Text {
                        text: "CV"
                        color: "#1DBF75"
                        font.bold: true
                        font.pixelSize: 18 * root.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 6 * root.scaleFactor
                        anchors.left: parent.left
                    }

                    Text {
                        color: "#A0A0B0"
                        anchors.centerIn: parent
                        text: root.cvSetpoint.toFixed(3) + " V"
                        font.bold: root.cvModel ? true : false
                        font.pixelSize: 18 * root.scaleFactor
                    }

                    Rectangle {
                        radius: width / 2
                        color: root.cvMode ? "#1DBF75" : "transparent"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 6 * root.scaleFactor
                        anchors.right: parent.right

                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        border.width: 1.8 * root.scaleFactor
                        border.color: "#1DBF75"
                    }
                }

                Item {
                    id: ccRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30 * root.scaleFactor

                    Text {
                        text: "CC"
                        color: "#FF3D52"
                        font.bold: true
                        font.pixelSize: 18 * root.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 6 * root.scaleFactor
                        anchors.left: parent.left
                    }

                    Text {
                        color: "#A0A0B0"
                        anchors.centerIn: parent
                        text: root.ccSetpoint.toFixed(3) + " A"
                        font.bold: root.ccModel ? true : false
                        font.pixelSize: 18 * root.scaleFactor
                    }

                    Rectangle {
                        radius: width / 2
                        color: root.ccMode ? "#FF3D52" : "transparent"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 6 * root.scaleFactor
                        anchors.right: parent.right

                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        border.width: 1.8 * root.scaleFactor
                        border.color: "#FF3D52"
                    }
                }

                Item {
                    id: ovRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30 * root.scaleFactor

                    Text {
                        text: "OVP"
                        color: "#E5C27D"
                        font.bold: true
                        font.pixelSize: 18 * root.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 6 * root.scaleFactor
                        anchors.left: parent.left
                    }

                    Text {
                        color: "#A0A0B0"
                        anchors.centerIn: parent
                        text: root.ovpSetpoint.toFixed(3) + " V"
                        font.bold: root.ovpModel ? true : false
                        font.pixelSize: 18 * root.scaleFactor
                    }

                    Rectangle {
                        radius: width / 2
                        color: root.ovpMode ? "#E5C27D" : "transparent"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 6 * root.scaleFactor
                        anchors.right: parent.right

                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        border.width: 1.8 * root.scaleFactor
                        border.color: "#E5C27D"
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            enabled: root.enclick
            pressAndHoldInterval: 600 // 0.6s

            onPressed: {
                root.pressed = true
                root.longpressed = false
                pressAnimation.start()
            }
            onReleased: {
                if (root.pressed) {
                    root.pressed = false
                    releaseAnimation.start()
                }
            }
            onCanceled: {
                root.pressed = false
                root.longpressed = false
                releaseAnimation.start()
            }

            onPressAndHold: {
                root.longpressed = true
                root.pressAndHold()
            }
            onClicked: {
                if (!root.longpressed) {
                    root.clicked()
                }
            }
        }
    }

    PropertyAnimation {
        id: pressAnimation
        target: card
        property: "scale"
        from: 1.0
        to: 0.96
        duration: 110
        easing.type: Easing.OutCubic
    }

    PropertyAnimation {
        id: releaseAnimation
        target: card
        property: "scale"
        from: 0.96
        to: 1.0
        duration: 110
        easing.type: Easing.OutElastic
        easing.amplitude: 0.2
        easing.period: 0.4
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:400;width:280}
}
##^##*/

