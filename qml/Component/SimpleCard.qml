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
    property real soc: 60

    signal batteryclicked
    signal batterypressAndHold
    signal digitalclicked
    signal digitalpressAndHold

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
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: parent.height * 0.18

                Text {
                    id: channelText
                    color: "#E0E0E0"
                    font.bold: true
                    font.pixelSize: 36 * root.scaleFactor
                    anchors.centerIn: parent
                    text: root.channelName
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
                Layout.fillHeight: true
                Layout.preferredHeight: parent.height * 0.36

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

                MouseArea {
                    id: digitalmouseArea
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
                        root.digitalpressAndHold()
                    }
                    onClicked: {
                        if (!root.longpressed) {
                            root.digitalclicked()
                        }
                    }
                }
            }

            Rectangle {
                id: batteryCard
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: parent.height * 0.27

                color: "#2A2A3C" //"transparent"
                border.color: "#4A6FA5"
                border.width: 1.8 * root.scaleFactor
                radius: 18 * root.scaleFactor

                GridLayout {
                    rows: 4
                    columns: 9
                    anchors.fill: parent
                    anchors.margins: 6.6 * root.scaleFactor
                    columnSpacing: 0.9 * root.scaleFactor
                    rowSpacing: 0.9 * root.scaleFactor

                    Repeater {
                        model: 36
                        Rectangle {
                            color: "#1E1E2E" //"#4A6FA5"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 1.8 * root.scaleFactor
                        }
                    }
                }

                Rectangle {
                    id: batteryFill
                    anchors {
                        left: parent.left
                        leftMargin: 3 * root.scaleFactor
                        verticalCenter: parent.verticalCenter
                    }
                    width: (parent.width - 6 * root.scaleFactor)
                           * ((root.soc / 100) > 1 ? 1.0 : (root.soc / 100))
                    height: batteryFill.width
                            < 18 ? parent.height - (18 - batteryFill.width)
                                   * 2 : (parent.height - 6 * root.scaleFactor)
                    radius: batteryFill.width < 18 ? batteryFill.width / 2 : 18 * root.scaleFactor
                    color: root.soc > 60 ? "#2ECC71" : (root.soc > 20 ? "#F39C12" : "#E74C3C")
                    opacity: 0.66
                }

                Text {
                    color: "#E0E0E0"
                    anchors.centerIn: parent
                    text: root.soc.toFixed(2) + " %"
                    font.pixelSize: 36 * root.scaleFactor
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        leftMargin: 9 * root.scaleFactor
                        verticalCenter: parent.verticalCenter
                    }
                    color: "#C0C0C0"
                    height: parent.height * 0.7
                    width: 1.8 * root.scaleFactor
                }

                Rectangle {
                    anchors {
                        right: parent.right
                        rightMargin: 9 * root.scaleFactor
                        verticalCenter: parent.verticalCenter
                    }
                    color: "#C0C0C0"
                    height: parent.height * 0.7
                    width: 1.8 * root.scaleFactor
                }

                MouseArea {
                    id: batterymouseArea
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
                        root.batterypressAndHold()
                    }
                    onClicked: {
                        if (!root.longpressed) {
                            root.batteryclicked()
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

