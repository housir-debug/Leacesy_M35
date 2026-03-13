import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    implicitWidth: 80
    implicitHeight: 80
    width: implicitWidth * scaleFactor
    height: implicitHeight * scaleFactor

    property real scaleFactor: 1.0
    property bool enclick: true
    property string mainText: "main"
    property string subText: "sub"
    property color mainTextColor: "#E0E0E0"
    property color subTextColor: "#8A9FB0"

    signal clicked
    signal pressAndHold

    property bool pressed: false
    property bool longpressed: false
    readonly property color colorBackground: "#0A1929"
    readonly property color colorBackgroundPressed: "#1E3A5F"
    readonly property color colorBorder: "#1E3A5F"
    readonly property color colorBorderPressed: "#4A9EFF"
    readonly property color colorGlow: "#4A9EFF"

    Rectangle {
        id: box_Button
        radius: 18 * root.scaleFactor
        anchors.fill: parent
        border.width: 1.2
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
            border.width: 1.8
            border.color: Qt.rgba(root.colorGlow.r, root.colorGlow.g,
                                  root.colorGlow.b, 0.15)
        }

        Rectangle {
            width: parent.width * 0.6
            height: 2
            color: root.colorGlow
            opacity: root.pressed ? 0.5 : 0.2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 6 * root.scaleFactor


            /*layer {
                enabled: root.pressed
                effect: DropShadow {
                    transparentBorder: true
                    color: root.colorGlow
                    radius: 8
                    samples: 16
                }
            }*/
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
                    if (root.mainText.length <= 4)
                        return baseSize
                    else if (root.mainText.length <= 6)
                        return baseSize * 0.9
                    else if (root.mainText.length <= 8)
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
                    if (root.subText.length <= 4)
                        return baseSize
                    else if (root.subText.length <= 6)
                        return baseSize * 0.9
                    else if (root.subText.length <= 8)
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
        property: "scaleFactor"
        from: root.scaleFactor
        to: root.scaleFactor * 0.97
        duration: 100
        easing.type: Easing.OutCubic
    }

    PropertyAnimation {
        id: releaseAnimation
        target: root
        property: "scaleFactor"
        from: root.scaleFactor * 0.97
        to: root.scaleFactor
        duration: 150
        easing.type: Easing.OutElastic
        easing.amplitude: 0.2
        easing.period: 0.4
    }
}
