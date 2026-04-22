import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: digitalhomePage

    signal toSettingPage
    signal toBatteryHomePage
    signal toFunctionPage(int value)

    property int totalChannels: 36
    property var existChannels: []
    property color backgroundcolor: "#0d1b2a" //"#0a0f1a"

    Component.onCompleted: {
        var channels = []
        for (var i = 1; i <= totalChannels; i++) {
            if (Uart_bridge["ch" + i + "_sv"] === "0.0.0.0") {
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

            delegate: DigitalCard {
                required property int index
                opacity: PathView.opacity
                scale: PathView.scale
                z: PathView.z || 0

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

            path: Path {
                // Starting point (0.0)
                startX: -36 //-18
                startY: cardPathView.height / 2
                PathAttribute {
                    name: "scale"
                    value: 0.81 // 0.69
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.72
                }
                PathAttribute {
                    name: "z"
                    value: 0
                }

                // middle
                PathLine {
                    x: cardPathView.width / 2
                    y: cardPathView.height / 2
                }
                PathAttribute {
                    name: "scale"
                    value: 0.96
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.9
                }
                PathAttribute {
                    name: "z"
                    value: 1
                }

                // terminus (1.0)
                PathLine {
                    x: cardPathView.width + 36 //+ 18
                    y: cardPathView.height / 2
                }
                PathAttribute {
                    name: "scale"
                    value: 0.81 // 0.69
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.72
                }
                PathAttribute {
                    name: "z"
                    value: 0
                }
            }

            pathItemCount: 7
            cacheItemCount: 6
            preferredHighlightBegin: 0.5 //Slide the final card to the center position
            preferredHighlightEnd: 0.5 //Slide the final card to the center position
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

