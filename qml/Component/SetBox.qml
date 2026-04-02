import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    implicitWidth: 80
    implicitHeight: 80

    property bool enclick: true
    property string mainText: "main"
    property string subText: "sub"
    property color mainTextColor: "#E0E0E0"
    property color subTextColor: "#8A9FB0"

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
    readonly property color colorBackground: "#0A1929"
    readonly property color colorBackgroundPressed: "#1E3A5F"
    readonly property color colorBorder: "#1E3A5F"
    readonly property color colorBorderPressed: "#4A9EFF"
    readonly property color colorGlow: "#4A9EFF"

    Rectangle {
        anchors.fill: parent
        radius: 18 * root.scaleFactor
        border.width: 1.2 * root.scaleFactor
        border.color: root.pressed ? root.colorBorderPressed : root.colorBorder
        color: root.pressed ? root.colorBackgroundPressed : root.colorBackground

        Rectangle {
            width: parent.width
            height: parent.height / 2
            radius: parent.radius
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(1, 1, 1, 0.15)
                }
                GradientStop {
                    position: 0.8
                    color: "transparent"
                }
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height / 2
            radius: parent.radius
            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: "transparent"
                }
                GradientStop {
                    position: 1.0
                    color: Qt.rgba(0, 0, 0, 0.2)
                }
            }
            anchors.bottom: parent.bottom
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius - 1.8
            color: "transparent"
            border.width: 1.8 * root.scaleFactor
            border.color: Qt.rgba(root.colorGlow.r, root.colorGlow.g,
                                  root.colorGlow.b, 0.15)
        }

        Rectangle {
            width: parent.width * 0.6
            height: 2 * root.scaleFactor
            color: root.colorGlow
            opacity: root.pressed ? 0.5 : 0.2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 6 * root.scaleFactor

            layer {
                enabled: root.pressed
                effect: DropShadow {
                    transparentBorder: true
                    color: root.colorGlow
                    radius: 8 * root.scaleFactor
                    samples: 16 * root.scaleFactor
                }
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 6 * root.scaleFactor

            Text {
                id: mainLabel
                color: root.mainTextColor
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.mainText !== ""
                text: root.mainText
                font.bold: true
                font.pixelSize: {
                    var baseSize = 18 * root.scaleFactor
                    if (root.mainText.length <= 9)
                        return baseSize
                    else if (root.mainText.length <= 12)
                        return baseSize * 0.9
                    else if (root.mainText.length <= 18)
                        return baseSize * 0.7
                    else
                        return baseSize * 0.4
                }
                style: Text.Raised
                styleColor: Qt.rgba(0, 0, 0, 0.3)
            }

            Text {
                id: subLabel
                color: root.subTextColor
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.subText !== ""
                text: root.subText
                font.bold: true
                font.pixelSize: {
                    var baseSize = 18 * root.scaleFactor
                    if (root.subText.length <= 9)
                        return baseSize
                    else if (root.subText.length <= 12)
                        return baseSize * 0.9
                    else if (root.subText.length <= 18)
                        return baseSize * 0.7
                    else
                        return baseSize * 0.4
                }

                style: Text.Raised
                styleColor: Qt.rgba(0, 0, 0, 0.3)
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
        target: root
        property: "scale"
        from: 1.0
        to: 0.96
        duration: 100
        easing.type: Easing.OutCubic
    }

    PropertyAnimation {
        id: releaseAnimation
        target: root
        property: "scale"
        from: 0.96
        to: 1.0
        duration: 150
        easing.type: Easing.OutElastic
        easing.amplitude: 0.2
        easing.period: 0.4
    }
}
