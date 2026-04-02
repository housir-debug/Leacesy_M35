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
    property real esr: 0.0
    property string esrUnit: "Ω"

    property string batteryModel: "model-1"
    property string workMode: "static" //Dynamic
    property real batteryCapacity: 50.0

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
            spacing: 9 * root.scaleFactor

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
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
                id: batteryCard
                radius: 24 * root.scaleFactor
                color: "#2A2A3C"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: parent.height * 0.27

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
            }

            GridLayout {
                id: infoGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: parent.height * 0.48
                columns: 2
                columnSpacing: 1.8 * root.scaleFactor
                rowSpacing: 1.8 * root.scaleFactor

                Repeater {
                    model: [{
                            "label": "电池模型",
                            "value": root.batteryModel,
                            "unit": ""
                        }, {
                            "label": "当前电压",
                            "value": root.voltage.toFixed(3),
                            "unit": root.voltageUnit
                        }, {
                            "label": "工作模式",
                            "value": root.workMode,
                            "unit": ""
                        }, {
                            "label": "当前电流",
                            "value": root.current.toFixed(3),
                            "unit": root.currentUnit
                        }, {
                            "label": "电池容量",
                            "value": root.batteryCapacity.toFixed(2),
                            "unit": "Ah"
                        }, {
                            "label": "当前内阻",
                            "value": root.esr.toFixed(3),
                            "unit": root.esrUnit
                        }]

                    Rectangle {
                        radius: 9 * root.scaleFactor
                        color: root.channelOutput ? "#151E2C" : "#181824"
                        border.color: "#2A3448"
                        border.width: 1.2 * root.scaleFactor
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Column {
                            anchors.centerIn: parent
                            spacing: 0.9 * root.scaleFactor

                            Text {
                                text: modelData.label
                                font.pixelSize: 12 * root.scaleFactor
                                font.family: "Microsoft YaHei"
                                font.weight: Font.Medium
                                color: "#8E9AFF"
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            Row {
                                spacing: 3.6 * root.scaleFactor
                                anchors.horizontalCenter: parent.horizontalCenter

                                Text {
                                    visible: modelData.value
                                    text: modelData.value
                                    font.pixelSize: 12 * root.scaleFactor
                                    font.family: "Microsoft YaHei"
                                    font.weight: Font.Bold
                                    color: "#FFFFFF"
                                }

                                Text {
                                    visible: modelData.unit
                                    text: modelData.unit
                                    font.pixelSize: 9 * root.scaleFactor
                                    font.family: "Microsoft YaHei"
                                    font.weight: Font.Medium
                                    color: "#8E9AFF"
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 1.2 * root.scaleFactor
                                }
                            }
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
    D{i:0;formeditorZoom:0.9}
}
##^##*/

