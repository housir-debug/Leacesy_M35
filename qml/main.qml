import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Component 1.0

ApplicationWindow {
    id: mainWindow
    width: 1024
    height: 800
    visible: true

    property int settingsChannel: 0
    property real maincvChange: 0.0
    property real mainccChange: 1.0
    property real mainovChange: 8.0

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
        }

        // Index: 1
        SettingPage {
            id: setting_page
            initialChannel: mainWindow.settingsChannel
            onBackRequested: {
                mainWindow.maincvChange = cvV
                mainWindow.mainccChange = ccV
                mainWindow.mainovChange = ovV
                stackLayout.currentIndex = 0
                changeHomechannelS()
            }
        }

        // Index: 2
        SystemPage {
            id: system_page
            onBackRequested: stackLayout.currentIndex = 0
        }
    }

    function changeHomechannelS() {
        switch (settingsChannel) {
        case 1:
            main_page.home_ch1cvChange = mainWindow.maincvChange
            main_page.home_ch1ccChange = mainWindow.mainccChange
            main_page.home_ch1ovChange = mainWindow.mainovChange
            return
        case 2:
            main_page.home_ch2cvChange = mainWindow.maincvChange
            main_page.home_ch2ccChange = mainWindow.mainccChange
            main_page.home_ch2ovChange = mainWindow.mainovChange
            return
        default:
            main_page.home_ch1cvChange = mainWindow.maincvChange
            main_page.home_ch1ccChange = mainWindow.mainccChange
            main_page.home_ch1ovChange = mainWindow.mainovChange
            main_page.home_ch2cvChange = mainWindow.maincvChange
            main_page.home_ch2ccChange = mainWindow.mainccChange
            main_page.home_ch2ovChange = mainWindow.mainovChange
            return
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.66}
}
##^##*/

