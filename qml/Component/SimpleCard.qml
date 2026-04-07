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
    property real soc: 100
    property real voltage: 0.0
    property string voltageUnit: "V"
    property real current: 0.0
    property string currentUnit: "A"

    signal batteryclicked
    signal batterypressAndHold
    signal digitalclicked
    signal digitalpressAndHold

    property real scaleFactor: 1.0


    /*property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }*/
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
            }

            Rectangle {
                id: batteryCard
                radius: 24 * root.scaleFactor
                color: "#2A2A3C"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: parent.height * 0.36

                Rectangle {
                    id: batteryBody
                    anchors {
                        fill: parent
                        margins: 3.6 * root.scaleFactor
                    }
                    radius: 24 * root.scaleFactor
                    color: "#1E1E2E"
                    border.color: "#4A6FA5"
                    border.width: 1.8 * root.scaleFactor
                    clip: true

                    Rectangle {
                        id: batteryFill
                        anchors {
                            left: parent.left
                            leftMargin: 1.8 * root.scaleFactor
                            verticalCenter: parent.verticalCenter
                        }
                        width: (parent.width - 3.6 * root.scaleFactor)
                               * ((root.soc / 100) > 1 ? 1.0 : (root.soc / 100))
                        height: batteryFill.width < 24 ? parent.height - (24 - batteryFill.width)
                                                         * 2 : parent.height
                        radius: batteryFill.width < 24 ? batteryFill.width / 2 : 24
                                                         * root.scaleFactor
                        color: root.soc > 60 ? "#2ECC71" : (root.soc > 20 ? "#F39C12" : "#E74C3C")
                    }

                    GridLayout {
                        anchors.fill: parent
                        columns: 9
                        rows: 4
                        opacity: 0.12
                        columnSpacing: 1.2 * root.scaleFactor
                        rowSpacing: 1.2 * root.scaleFactor

                        Repeater {
                            model: 36
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#4A6FA5"
                            }
                        }
                    }

                    Rectangle {
                        anchors {
                            left: parent.left
                            top: parent.top
                            bottom: parent.bottom
                            margins: 12 * root.scaleFactor
                        }
                        width: 3.6 * root.scaleFactor
                        radius: 1.8 * root.scaleFactor
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: "#C0C0C0"
                            }
                            GradientStop {
                                position: 0.5
                                color: "#E8E8E8"
                            }
                            GradientStop {
                                position: 1.0
                                color: "#C0C0C0"
                            }
                        }
                    }

                    Rectangle {
                        anchors {
                            right: parent.right
                            top: parent.top
                            bottom: parent.bottom
                            margins: 12 * root.scaleFactor
                        }
                        width: 3.6 * root.scaleFactor
                        radius: 1.8 * root.scaleFactor
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: "#C0C0C0"
                            }
                            GradientStop {
                                position: 0.5
                                color: "#E8E8E8"
                            }
                            GradientStop {
                                position: 1.0
                                color: "#C0C0C0"
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: root.soc.toFixed(2) + " %"
                    font.pixelSize: 30 * root.scaleFactor
                    font.bold: true
                    font.family: "Microsoft YaHei"
                    color: root.soc > 30 ? "#FFFFFF" : "#E0E0E0"
                    style: Text.Outline
                    styleColor: "#80000000"
                    z: 1
                }

                MouseArea {
                    id: batterymouseArea
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
                        root.batterypressAndHold()
                    }
                    onClicked: {
                        if (!root.longpressed) {
                            root.batteryclicked()
                        }
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
                Layout.preferredHeight: parent.height * 0.36

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

                MouseArea {
                    id: digitalmouseArea
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
                        root.digitalpressAndHold()
                    }
                    onClicked: {
                        if (!root.longpressed) {
                            root.digitalclicked()
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
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

