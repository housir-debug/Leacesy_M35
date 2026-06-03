import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Item {
    id: root

    property bool enclick: true
    property string buttonText: "Remote   >>>   Local"

    signal unlock

    implicitWidth: 360
    implicitHeight: 81

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }
    property real sliderX: 9 * scaleFactor

    Rectangle {
        id: backgroundRect
        radius: height / 2
        anchors.fill: parent
        border.color: "#909090"
        border.width: 6.4 * root.scaleFactor
        color: "#AAB1BF"

        Rectangle {
            anchors.fill: parent
            radius: parent.radius

            gradient: Gradient {
                GradientStop {
                    position: 0.18
                    color: Qt.rgba(0, 0, 0, 0.36)
                }
                GradientStop {
                    position: 0.81
                    color: Qt.rgba(180, 180, 180, 0.36)
                }
            }
        }

        Text {
            id: labelText
            color: "#6a7275"
            text: root.buttonText
            anchors.centerIn: parent
            font.pixelSize: 18 * root.scaleFactor
            font.bold: true
        }

        Item {
            id: sliderContainer
            width: (parent.height - 18) * root.scaleFactor
            height: (parent.height - 18) * root.scaleFactor
            y: (parent.height - height) / 2 * root.scaleFactor
            x: root.sliderX

            Rectangle {
                id: sliderRect
                radius: width / 2
                anchors.fill: parent
                color: Qt.rgba(9, 9, 9, 0.36) //"transparent"

                border.color: Qt.rgba(180, 180, 180, 0.9)
                border.width: 1.8 * root.scaleFactor
            }

            Image {
                anchors.centerIn: parent
                width: 24 * root.scaleFactor
                height: 24 * root.scaleFactor
                source: "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1024 1024'><path fill='%237a858a' stroke='%237a858a' stroke-width='36' d='M800 960H224c-52.8 0-96-43.2-96-96V480c0-52.8 43.2-96 96-96h576c52.8 0 96 43.2 96 96v384c0 52.8-43.2 96-96 96zM224 448c-17.6 0-32 14.4-32 32v384c0 17.6 14.4 32 32 32h576c17.6 0 32-14.4 32-32V480c0-17.6-14.4-32-32-32H224z m528-32c-17.6 0-32-14.4-32-32V272c0-115.2-92.8-208-208-208s-208 92.8-208 208v112c0 17.6-14.4 32-32 32s-32-14.4-32-32V272C240 121.6 361.6 0 512 0s272 121.6 272 272v112c0 17.6-14.4 32-32 32z'/></svg>"
                fillMode: Image.PreserveAspectFit
            }
        }
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        enabled: root.enclick
        drag.target: sliderContainer
        drag.axis: Drag.XAxis
        drag.minimumX: minX
        drag.maximumX: maxX
        drag.threshold: 0

        readonly property real minX: 9 * root.scaleFactor
        readonly property real maxX: parent.width - (sliderContainer.width + 6.4 * root.scaleFactor)

        onPositionChanged: {
            root.sliderX = sliderContainer.x
            if (sliderContainer.x >= maxX * 0.81) {
                //root.sliderX = maxX
                root.sliderX = minX
                root.unlock()
            }
        }

        onReleased: {
            root.sliderX = minX
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorColor:"#ffffff"}
}
##^##*/

