import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    id: mainWindow
    width: Screen.desktopAvailableWidth
    height: Screen.desktopAvailableHeight
    color: "#0d1b2a" //"#0a0f1a"
    visibility: "FullScreen"
    visible: true

    property int homePageModel: 0
    property int functionChannel: 0

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0

        // Index: 0
        DigitalHomePage {
            id: digitalmain_page
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

            Component.completed: {
                Uart_bridge.load_BatteryModel()
            }
        }

        // Index: 2
        SettingPage {
            id: setting_page
            onBackRequested: {
                if (homePageModel === 0) {
                    stackLayout.currentIndex = 0
                } else {
                    stackLayout.currentIndex = 1
                }
            }
        }

        // Index: 3
        FunctionPage {
            id: function_page
            initialChannel: mainWindow.functionChannel
            onBackRequested: {
                if (homePageModel === 0) {
                    stackLayout.currentIndex = 0
                } else {
                    stackLayout.currentIndex = 1
                }
            }
        }
    }

    Rectangle {
        id: remoteOverlay
        anchors.fill: parent
        color: "#80000000" // Transparency
        visible: Uart_bridge.isRemote
        z: 100

        Image {
            id: logoImage
            anchors.centerIn: parent
            width: parent.width * 0.5
            height: parent.height * 0.5
            source: "logo.png"
            fillMode: Image.PreserveAspectFit
            opacity: 0.7
        }

        Text {
            anchors.top: logoImage.bottom
            anchors.topMargin: 20
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Remote Mode - Double tap to exit"
            color: "white"
            font.pixelSize: 16
            opacity: 0.8
        }

        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                // default 200ms
                Uart_bridge.update_remotemodel(false)
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}
}
##^##*/

