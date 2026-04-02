import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: digitalhomePage

    signal toFunctionPage(int value)
    signal toSettingPage
    signal toBatteryHomePage

    property color backgroundcolor: "#0d1b2a" //"#0a0f1a"
    property int totalChannels: 36
    property var existChannels: []

    Component.onCompleted: {
        var channels = []
        for (var i = 1; i <= totalChannels; i++) {
            if (Uart_bridge["ch" + i + "_sv"] !== "0.0.0.0"
                    && Uart_bridge["ch" + i + "_hv"] !== "0.0.0.0") {

                channels.push(i)
            }
        }
        existChannels = channels
    }

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        PathView {
            id: cardPathView
            anchors.fill: parent
            model: existChannels

            delegate: Item {
                width: cardPathView.cardWidth
                height: cardPathView.cardHeight
                scale: PathView.scale
                opacity: PathView.opacity
                z: PathView.z

                DigitalCard {
                    required property int index
                    anchors.fill: parent

                    enclick: !Uart_bridge.isRemote
                    channelOutput: Uart_bridge["ch" + existChannels[index] + "_isOutput"]

                    channelName: "CH" + existChannels[index]
                    voltage: Uart_bridge["ch" + existChannels[index] + "_Voltage"]
                    voltageUnit: "V"
                    current: Uart_bridge["ch" + existChannels[index] + "_Current"]
                    currentUnit: Uart_bridge["ch" + existChannels[index] + "_CurrentUnit"]

                    cvSetpoint: Uart_bridge["ch" + existChannels[index] + "_cv"]
                    cvMode: Uart_bridge["ch" + existChannels[index] + "_Status"].charAt(
                                14) === "1"
                    ccSetpoint: Uart_bridge["ch" + existChannels[index] + "_cc"]
                    ccMode: Uart_bridge["ch" + existChannels[index] + "_Status"].charAt(
                                13) === "1"
                    ovpSetpoint: Uart_bridge["ch" + existChannels[index] + "_ovp"]
                    ovpMode: Uart_bridge["ch" + existChannels[index] + "_Status"].charAt(
                                 11) === "1"

                    onClicked: {
                        Uart_bridge.setChannel_Output(existChannels[index],
                                                      !channelOutput)
                    }

                    onPressAndHold: {
                        toFunctionPage(existChannels[index])
                    }
                }
            }

            path: Path {
                startX: -cardWidth / 2 // Left outer edge of the screen
                startY: cardPathView.height / 2
                PathLine {
                    x: cardPathView.width + cardWidth / 2 // The outer right side of the screen
                    y: cardPathView.height / 2
                }

                // Starting point (0.0)
                PathAttribute {
                    name: "scale"
                    value: 0.65
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.5
                }

                // midpoint (0.5)
                PathAttribute {
                    name: "scale"
                    value: 1.0
                }
                PathAttribute {
                    name: "opacity"
                    value: 1.0
                }

                // terminus (1.0)
                PathAttribute {
                    name: "scale"
                    value: 0.65
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.5
                }
            }

            property real cardWidth: 280
            property real cardHeight: parent.height - 18
            pathItemCount: Math.min(7, existChannels.length * 2 + 1) // odd
            preferredHighlightBegin: 0.5 //Slide the final card to the center position
            preferredHighlightEnd: 0.5 //Slide the final card to the center position
            //dragMargin: width
        }
    }

    MouseArea {
        id: bottomEdgeSwipe
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        property real startY: 0

        onPressed: startY = mouseY
        onReleased: {
            var delta = startY - mouseY
            if (delta > 108) {
                toBatteryHomePage()
            }
        }
    }

    MouseArea {
        id: topEdgeSwipe
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        property real startY: 0

        onPressed: startY = mouseY
        onReleased: {
            var delta = mouseY - startY
            if (delta > 108) {
                toSettingPage()
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.75;height:360;width:960}
}
##^##*/

