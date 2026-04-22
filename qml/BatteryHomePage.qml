import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: batteryhomePage

    signal toSettingPage
    signal toDigitalHomePage
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
        Uart_bridge.load_BatteryModel()
    }

    Rectangle {
        anchors.fill: parent
        color: backgroundcolor

        PathView {
            id: cardPathView
            anchors.fill: parent
            model: existChannels

            delegate: BatteryCard {
                required property int index
                opacity: PathView.opacity
                scale: PathView.scale
                z: PathView.z || 0

                enclick: !Uart_bridge.isRemote
                channelOutput: Uart_bridge["ch" + existChannels[index] + "_isOutput"]

                channelName: "CH" + existChannels[index]
                soc: Uart_bridge["ch" + existChannels[index] + "_CurrentSOC"]
                ocv: Uart_bridge["ch" + existChannels[index] + "_cv"]
                ocvUnit: "V"
                esr: Uart_bridge["ch" + existChannels[index] + "_imp"]
                esrUnit: "Ω"

                batteryModel: Uart_bridge["ch" + existChannels[index] + "_BatteryModel"]
                batteryCapacity: Uart_bridge["ch" + existChannels[index] + "_CapacityAH"]

                onClicked: {
                    Uart_bridge.setChannel_BatteryOutput(existChannels[index],
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
                    x: cardPathView.width + 18
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
                toSettingPage()
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
                toDigitalHomePage()
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.75;height:360;width:960}
}
##^##*/

