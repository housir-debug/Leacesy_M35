import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    implicitWidth: 280
    implicitHeight: 400

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

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }
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
        border.width: 2 * root.scaleFactor
        border.color: root.channelOutput ? root.colorGlow : root.colorCardBorder
        color: root.channelOutput ? "#353548" : "#181824"

        Rectangle {
            width: parent.width * 0.7
            height: 1 * root.scaleFactor
            color: root.colorGlow
            opacity: 0.4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 8 * root.scaleFactor
        }
        Rectangle {
            width: parent.width * 0.7
            height: 1 * root.scaleFactor
            color: root.colorGlow
            opacity: 0.4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8 * root.scaleFactor
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18 * root.scaleFactor
            spacing: 16 * root.scaleFactor

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * 0.18
                Layout.alignment: Qt.AlignTop

                Text {
                    id: channelText
                    color: root.textPrimary
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
                    border.width: 1 * root.scaleFactor
                    border.color: "#000000"

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width + 4 * root.scaleFactor
                        height: parent.height + 4 * root.scaleFactor
                        radius: width / 2
                        color: "transparent"
                        border.width: 1 * root.scaleFactor
                        border.color: root.channelOutput ? root.colorGlow : "transparent"
                        opacity: 0.6
                    }
                }
            }

            Rectangle {
                id: measurementCard
                radius: 16 * root.scaleFactor
                color: "#080810"
                border.width: 1.2 * root.scaleFactor
                border.color: "#3A3A4E"
                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * 0.4

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

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12 * root.scaleFactor

                    Text {
                        id: voltageText
                        color: root.colorCv
                        font.bold: true
                        font.pixelSize: 36 * root.scaleFactor
                        Layout.alignment: Qt.AlignHCenter
                        text: root.voltage.toFixed(4) + " " + root.voltageUnit
                        style: Text.Raised
                        styleColor: "#000000"
                    }

                    Text {
                        id: currentText
                        color: root.colorCc
                        font.bold: true
                        font.pixelSize: (root.currentUnit == "mA" ? 30 : 36) * root.scaleFactor
                        Layout.alignment: Qt.AlignHCenter
                        text: root.current.toFixed(4) + " " + root.currentUnit
                        style: Text.Raised
                        styleColor: "#000000"
                    }
                }

                Rectangle {
                    width: parent.width - 20 * root.scaleFactor
                    height: 2 * root.scaleFactor
                    color: root.colorGlow
                    opacity: 0.3
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5 * root.scaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
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
                        color: root.colorCv
                        font.bold: true
                        font.pixelSize: 20 * root.scaleFactor
                        style: Text.Raised
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 3 * root.scaleFactor
                    }

                    Text {
                        text: root.cvSetpoint.toFixed(3) + " V"
                        color: root.textSecondary
                        font.pixelSize: 15 * root.scaleFactor
                        font.bold: root.cvModel ? true : false
                        anchors.centerIn: parent
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        radius: width / 2
                        border.width: 2 * root.scaleFactor
                        border.color: root.colorCv
                        color: root.cvMode ? root.colorCv : "transparent"
                        anchors.right: parent.right
                        anchors.rightMargin: 3 * root.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item {
                    id: ccRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30 * root.scaleFactor

                    Text {
                        text: "CC"
                        color: root.colorCc
                        font.bold: true
                        font.pixelSize: 20 * root.scaleFactor
                        style: Text.Raised
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 3 * root.scaleFactor
                    }

                    Text {
                        text: root.ccSetpoint.toFixed(3) + " A"
                        color: root.textSecondary
                        font.pixelSize: 15 * root.scaleFactor
                        font.bold: root.ccModel ? true : false
                        anchors.centerIn: parent
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        radius: width / 2
                        border.width: 2 * root.scaleFactor
                        border.color: root.colorCc
                        color: root.ccMode ? root.colorCc : "transparent"
                        anchors.right: parent.right
                        anchors.rightMargin: 3 * root.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item {
                    id: ovRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30 * root.scaleFactor

                    Text {
                        text: "OVP"
                        color: root.colorOv
                        font.bold: true
                        font.pixelSize: 20 * root.scaleFactor
                        style: Text.Raised
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 3 * root.scaleFactor
                    }

                    Text {
                        text: root.ovpSetpoint.toFixed(3) + " V"
                        color: root.textSecondary
                        font.pixelSize: 15 * root.scaleFactor
                        font.bold: root.ovpModel ? true : false
                        anchors.centerIn: parent
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 22 * root.scaleFactor
                        height: 22 * root.scaleFactor
                        radius: width / 2
                        border.width: 2 * root.scaleFactor
                        border.color: root.colorOv
                        color: root.ovpMode ? root.colorOv : "transparent"
                        anchors.right: parent.right
                        anchors.rightMargin: 3 * root.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
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
        to: 0.96
        duration: 100
        easing.type: Easing.OutCubic
    }

    PropertyAnimation {
        id: releaseAnimation
        target: card
        property: "scale"
        from: 0.96
        to: 1.0
        duration: 120
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

