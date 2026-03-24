import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    id: mainWindow
    width: Screen.desktopAvailableWidth
    height: Screen.desktopAvailableHeight
    visibility: "FullScreen"
    visible: true

    property int settingsChannel: 0
    property int functionChannel: 0

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0

        // Index: 0
        HomePage {
            id: main_page

            onToSettingPage: {
                //console.log("当前点击值：" + value)
                mainWindow.settingsChannel = value
                stackLayout.currentIndex = 1
            }
            onToSystemPage: stackLayout.currentIndex = 2
            onToFunctionPage: {
                mainWindow.functionChannel = value
                stackLayout.currentIndex = 3
            }
        }

        // Index: 1
        SettingPage {
            id: setting_page
            initialChannel: mainWindow.settingsChannel
            onBackRequested: stackLayout.currentIndex = 0
        }

        // Index: 2
        SystemPage {
            id: system_page
            onBackRequested: stackLayout.currentIndex = 0
        }

        // Index: 3
        FunctionPage {
            id: function_page
            initialChannel: mainWindow.functionChannel
            onBackRequested: stackLayout.currentIndex = 0
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}
}
##^##*/

