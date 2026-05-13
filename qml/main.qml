import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    id: mainWindow
    //width: Screen.desktopAvailableWidth
    //height: Screen.desktopAvailableHeight
    width: Screen.desktopAvailableHeight
    height: Screen.desktopAvailableWidth
    visibility: "FullScreen"
    color: "#0d1b2a" //"#0a0f1a"
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
        }

        // Index: 2
        SettingPage {
            id: setting_page
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
            backgroundcolor: mainWindow.color
            initialChannel: mainWindow.functionChannel

            onBackRequested: {
                if (homePageModel === 0) {
                    stackLayout.currentIndex = 0 // to digitalmain_page
                } else {
                    stackLayout.currentIndex = 1 // to batterymain_page
                }
            }
        }
    }

    Rectangle {
        z: 100
        id: remoteOverlay
        anchors.fill: parent
        color: "#90000000" //"#FF000000" // Translucent or black
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

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Remote Mode - Double tap to Exit"
            anchors.top: logoImage.bottom
            font.pixelSize: 18
            color: "#FFD700" //yellow or "red"
            font.bold: true
            opacity: 0.81
        }

        MouseArea {
            anchors.fill: parent
            enabled: Uart_bridge.reface !== 0

            onDoubleClicked: {
                // default 200ms
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

