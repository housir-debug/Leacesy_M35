import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 100
    height: 100

    signal clicked

    property string mainText: ""
    property string subText: ""
    property bool is_pressed: false
    property bool is_enclick: true
    property bool is_subTcolor_s: false
    property real scale: 1.0
    transform: Scale {
        origin.x: 0
        origin.y: 0
        xScale: root.scale
        yScale: root.scale
    }

    Rectangle {
        id: box_Button
        radius: 18
        anchors.fill: parent
        border.width: 6
        border.color: root.is_pressed ? "#8e949a" : "#ced4da"
        color: root.is_pressed ? "#3e444a" : "#f8f9fa"

        Column {
            anchors.centerIn: parent
            spacing: 6

            Text {
                id: mainLabel
                color: "#212529"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.mainText !== ""
                text: root.mainText
                font.bold: true
                font.pixelSize: {
                    if (root.mainText.length <= 4)
                        return 24
                    else if (root.mainText.length <= 8)
                        return 18
                    else
                        return 12
                }
            }

            Text {
                id: subLabel
                color: is_subTcolor_s ? "#8b0000" : "#6a9955"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.subText !== ""
                text: root.subText
                font.bold: true
                font.pixelSize: {
                    if (root.subText.length <= 4)
                        return 24
                    else if (root.subText.length <= 8)
                        return 18
                    else
                        return 12
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: box_Button
            enabled: root.is_enclick

            onPressed: {
                root.is_pressed = true
                pressAnimation.start()
            }

            onReleased: {
                root.is_pressed = false
                releaseAnimation.start()
                root.clicked()
            }
        }
    }

    ScaleAnimator {
        id: pressAnimation
        target: root
        from: 1.0
        to: 0.99
        duration: 66
        easing.type: Easing.OutCubic
    }

    ScaleAnimator {
        id: releaseAnimation
        target: root
        from: 0.99
        to: 1.0
        duration: 66
        easing.type: Easing.OutCubic
    }
}
