import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    implicitWidth: 280
    implicitHeight: 400
    width: implicitWidth * scaleFactor
    height: implicitHeight * scaleFactor

    property real scaleFactor: 1.0
    property bool enclick: true
    property bool channelOutput: false
    property string channelName: "CH1"
    property real voltage: 0.0
    property real current: 0.0
    property string voltageUnit: "V"
    property string currentUnit: "A"
    property bool cvModel: false
    property bool ccModel: false
    property bool ovpModel: false
    property real cvSetpoint: 0.0
    property real ccSetpoint: 1.0
    property real ovpSetpoint: 8.0

    signal clicked
    signal pressAndHold

    property bool pressed: false
    property bool longpressed: false
    readonly property color colorBackground: "#1E1E2E"
    readonly property color colorCardBorder: "#2B2B3C"
    readonly property color colorGlow: "#4A6FA5"
    readonly property color colorCv: "#1DBF75"
    readonly property color colorCc: "#FF3D52"
    readonly property color colorOv: "#E5C27D"
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#A0A0B0"

    Rectangle {
        id: card
        radius: 32 * root.scaleFactor
        anchors.fill: parent
        border.width: 2
        border.color: root.channelOutput ? colorGlow : colorCardBorder
        color: root.channelOutput ? "#353548" : "#181824"

        Rectangle {
            width: parent.width * 0.7
            height: 1
            color: colorGlow
            opacity: 0.4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 8 * root.scaleFactor
        }
        Rectangle {
            width: parent.width * 0.7
            height: 1
            color: colorGlow
            opacity: 0.4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8 * root.scaleFactor
        }

        Column {
            anchors.fill: parent
            anchors.margins: 18 * root.scaleFactor
            spacing: 16 * root.scaleFactor

            Item {
                width: parent.width
                height: parent.height * 0.18

                Text {
                    id: channelText
                    color: textPrimary
                    font.bold: true
                    font.pixelSize: 36 * root.scaleFactor
                    anchors.centerIn: parent
                    text: root.channelName
                    style: Text.Raised
                    styleColor: "#000000"
                }

                Rectangle {
                    id: channelDot
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                    }
                    width: 36 * root.scaleFactor
                    height: 36 * root.scaleFactor
                    radius: width / 2
                    color: root.channelOutput ? "#1AF080" : "#3A3A4E"
                    border.width: 1
                    border.color: "#000000"

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width + 4 * root.scaleFactor
                        height: parent.height + 4 * root.scaleFactor
                        radius: width / 2
                        color: "transparent"
                        border.width: 1
                        border.color: root.channelOutput ? colorGlow : "transparent"
                        opacity: 0.6
                    }
                }
            }

            Rectangle {
                id: measurementCard
                radius: 16 * root.scaleFactor
                color: "#080810"
                border.width: 1.2
                border.color: "#3A3A4E"
                width: parent.width
                height: parent.height * 0.4

                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop {
                        position: 0.0
                        color: "#14141E"
                    }
                    GradientStop {
                        position: 1.0
                        color: "#0A0A12"
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 12 * root.scaleFactor

                    Text {
                        id: voltageText
                        color: root.colorCv
                        font.bold: true
                        font.pixelSize: 36 * root.scaleFactor
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root.voltage.toFixed(4) + " " + root.voltageUnit
                        style: Text.Raised
                        styleColor: "#000000"
                    }

                    Text {
                        id: currentText
                        color: root.colorCc
                        font.bold: true
                        font.pixelSize: (root.currentUnit == "mA" ? 30 : 36) * root.scaleFactor
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root.current.toFixed(4) + " " + root.currentUnit
                        style: Text.Raised
                        styleColor: "#000000"
                    }
                }

                Rectangle {
                    width: parent.width - 20 * root.scaleFactor
                    height: 2
                    color: colorGlow
                    opacity: 0.3
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5 * root.scaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Column {
                id: setpointsColumn
                width: parent.width
                height: parent.height * 0.3
                spacing: 6 * root.scaleFactor

                Item {
                    id: cvRow
                    width: parent.width * 0.96
                    height: parent.height * 0.3
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "CV"
                        color: root.colorCv
                        font.bold: true
                        font.pixelSize: 20 * root.scaleFactor
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        style: Text.Raised
                    }

                    Text {
                        text: root.cvSetpoint.toFixed(3) + " V"
                        anchors.centerIn: parent
                        color: textSecondary
                        font.pixelSize: 15 * root.scaleFactor
                        font.bold: root.cvModel
                    }

                    Rectangle {
                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        radius: width / 2
                        border.width: 2
                        border.color: root.colorCv
                        color: root.cvModel ? root.colorCv : "transparent"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        Rectangle {
                            anchors.centerIn: parent
                            width: 8 * root.scaleFactor
                            height: 8 * root.scaleFactor
                            radius: width / 2
                            color: root.colorCv
                            visible: root.cvModel
                        }
                    }
                }

                Item {
                    id: ccRow
                    width: parent.width * 0.96
                    height: parent.height * 0.3
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "CC"
                        color: root.colorCc
                        font.bold: true
                        font.pixelSize: 20 * root.scaleFactor
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        style: Text.Raised
                    }

                    Text {
                        text: root.ccSetpoint.toFixed(3) + " A"
                        anchors.centerIn: parent
                        color: textSecondary
                        font.pixelSize: 15 * root.scaleFactor
                        font.bold: root.ccModel
                    }

                    Rectangle {
                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        radius: width / 2
                        border.width: 2
                        border.color: root.colorCc
                        color: root.ccModel ? root.colorCc : "transparent"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        Rectangle {
                            anchors.centerIn: parent
                            width: 8 * root.scaleFactor
                            height: 8 * root.scaleFactor
                            radius: width / 2
                            color: root.colorCc
                            visible: root.ccModel
                        }
                    }
                }

                Item {
                    id: ovRow
                    width: parent.width * 0.96
                    height: parent.height * 0.3
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "OVP"
                        color: root.colorOv
                        font.bold: true
                        font.pixelSize: 20 * root.scaleFactor
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        style: Text.Raised
                    }

                    Text {
                        text: root.ovpSetpoint.toFixed(3) + " V"
                        anchors.centerIn: parent
                        color: textSecondary
                        font.pixelSize: 15 * root.scaleFactor
                        font.bold: root.ovpModel
                    }

                    Rectangle {
                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        radius: width / 2
                        border.width: 2
                        border.color: root.colorOv
                        color: root.ovpModel ? root.colorOv : "transparent"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        Rectangle {
                            anchors.centerIn: parent
                            width: 8 * root.scaleFactor
                            height: 8 * root.scaleFactor
                            radius: width / 2
                            color: root.colorOv
                            visible: root.ovpModel
                        }
                    }
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            enabled: root.enclick
            pressAndHoldInterval: 1000

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
        to: 0.98
        duration: 100
        easing.type: Easing.OutCubic
    }

    PropertyAnimation {
        id: releaseAnimation
        target: card
        property: "scale"
        from: 0.98
        to: 1.0
        duration: 120
        easing.type: Easing.OutElastic
        easing.amplitude: 0.2
        easing.period: 0.4
    }
}
