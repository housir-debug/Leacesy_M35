import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

Item {
    id: batteryhomePage

    signal toSettingPage
    signal toDigitalHomePage
    signal toFunctionPage(int value)

    property bool enclick: true
    property color backgroundcolor: "#0d1b2a"
    property var existChannels: Uart_bridge.getActiveChannels()

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

                enclick: batteryhomePage.enclick
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
                // Starting point
                startX: -120
                startY: cardPathView.height / 2
                PathAttribute {
                    name: "scale"
                    value: 0.81
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.81
                }
                PathAttribute {
                    name: "z"
                    value: 0
                }

                // middle point
                PathLine {
                    x: cardPathView.width / 2
                    y: cardPathView.height / 2
                }
                PathAttribute {
                    name: "scale"
                    value: 0.98
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.98
                }
                PathAttribute {
                    name: "z"
                    value: 1
                }

                // terminus (1.0)
                PathLine {
                    x: cardPathView.width + 120
                    y: cardPathView.height / 2
                }
                PathAttribute {
                    name: "scale"
                    value: 0.81
                }
                PathAttribute {
                    name: "opacity"
                    value: 0.81
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
        enabled: batteryhomePage.enclick
        anchors.bottom: parent.bottom
        width: parent.width
        height: 36

        property real startY: 0
        onPressed: startY = mouseY
        onReleased: {
            if (startY - mouseY > 81) {
                toSettingPage()
            }
        }
    }

    MouseArea {
        id: topEdgeSwipe
        enabled: batteryhomePage.enclick
        anchors.top: parent.top
        width: parent.width
        height: 36

        property real startY: 0
        onPressed: startY = mouseY
        onReleased: {
            if (mouseY - startY > 81) {
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

