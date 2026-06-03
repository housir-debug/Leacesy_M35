import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    id: mainWindow
    visibility: "FullScreen"
    color: "#0d1b2a" //"#0a0f1a"
    visible: true

    property int homePageModel: 0
    property int functionChannel: 1

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0

        // Index: 0
        DigitalHomePage {
            id: digitalmain_page
            enclick: Uart_bridge.reface === 0
            backgroundcolor: mainWindow.color

            onToBatteryHomePage: {
                mainWindow.homePageModel = 0
                stackLayout.currentIndex = 1 // to batterymain_page
            }

            onToSettingPage: {
                mainWindow.homePageModel = 0
                stackLayout.currentIndex = 2 // to setting_page
            }

            onToFunctionPage: {
                mainWindow.homePageModel = 0
                stackLayout.currentIndex = 3 // to function_page
                mainWindow.functionChannel = value
            }
        }

        // Index: 1
        BatteryHomePage {
            id: batterymain_page
            enclick: Uart_bridge.reface === 0
            backgroundcolor: mainWindow.color

            onToDigitalHomePage: {
                mainWindow.homePageModel = 1
                stackLayout.currentIndex = 0 // to digitalmain_page
            }

            onToSettingPage: {
                mainWindow.homePageModel = 1
                stackLayout.currentIndex = 2 // to setting_page
            }

            onToFunctionPage: {
                mainWindow.homePageModel = 1
                stackLayout.currentIndex = 3 // to function_page
                mainWindow.functionChannel = value
            }
        }

        // Index: 2
        SettingPage {
            id: setting_page
            enclick: Uart_bridge.reface === 0
            backgroundcolor: mainWindow.color

            onToDigitalHomePage: {
                stackLayout.currentIndex = 0 // to digitalmain_page
            }

            onToBatteryHomePage: {
                stackLayout.currentIndex = 1 // to batterymain_page
            }
        }

        // Index: 3
        FunctionPage {
            id: function_page
            enclick: Uart_bridge.reface === 0
            initialChannel: mainWindow.functionChannel
            backgroundcolor: mainWindow.color

            onBackRequested: {
                if (mainWindow.homePageModel === 0) {
                    stackLayout.currentIndex = 0 // to digitalmain_page
                } else {
                    stackLayout.currentIndex = 1 // to batterymain_page
                }
            }
        }
    }

    Rectangle {
        z: 100
        color: "#90000000"
        anchors.fill: parent
        visible: Uart_bridge.reface !== 0

        Image {
            id: logoImage
            opacity: 0.69
            anchors.centerIn: parent
            width: parent.width * 0.81
            height: parent.height * 0.81
            fillMode: Image.PreserveAspectFit
            source: "qrc:/web/web/icon/leacesyicon.png"
        }

        Unlock {
            enabled: Uart_bridge.reface !== 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom

            onUnlock: {
                Uart_bridge.update_remotemodel(0)
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}
}
##^##*/

