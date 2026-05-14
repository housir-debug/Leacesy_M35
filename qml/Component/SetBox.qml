import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

Item {
    id: root

    property bool enclick: true
    property string mainText: "main"
    property string subText: "sub"
    property color mainTextColor: "#C0C0C0"
    property color subTextColor: "#8A9FB0"

    signal clicked
    signal pressAndHold

    implicitWidth: 90
    implicitHeight: 90

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
        anchors.fill: parent
        radius: 18 * root.scaleFactor
        border.width: 1.8 * root.scaleFactor
        border.color: root.pressed ? "#4A9EFF" : "#2E4A6F"
        color: root.pressed ? "#1E3A5F" : "#0A1929"

        Rectangle {
            anchors.fill: parent
            radius: 18 * root.scaleFactor
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(1, 1, 1, 0.12)
                }
                GradientStop {
                    position: 0.5
                    color: "transparent"
                }
                GradientStop {
                    position: 1.0
                    color: Qt.rgba(0, 0, 0, 0.18)
                }
            }
        }

        Rectangle {
            width: parent.width * 0.6
            height: 1.8 * root.scaleFactor
            color: root.pressed ? "#2E4A6F" : "#092754"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 6.9 * root.scaleFactor
            anchors.bottom: parent.bottom
        }

        Column {
            anchors.centerIn: parent
            spacing: 6 * root.scaleFactor

            Text {
                id: mainLabel
                text: root.mainText
                color: root.mainTextColor
                visible: root.mainText !== ""
                anchors.horizontalCenter: parent.horizontalCenter

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
            }

            Text {
                id: subLabel
                text: root.subText
                color: root.subTextColor
                visible: root.subText !== ""
                anchors.horizontalCenter: parent.horizontalCenter

                //font.bold: true
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
        target: root
        property: "scale"
        from: 1.0
        to: 0.96
        duration: 110
        easing.type: Easing.OutCubic
    }

    PropertyAnimation {
        id: releaseAnimation
        target: root
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
    D{i:0;autoSize:true;height:90;width:90}
}
##^##*/

