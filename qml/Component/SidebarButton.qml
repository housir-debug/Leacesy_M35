import QtQuick 2.15
import QtQuick.Controls 2.15

Item {

    Rectangle {
        id: root
        width: 240
        height: parent.height
        color: "#2d2d30"

        property int currentIndex: 0
        signal categorySelected(string category)

        Column {
            width: parent.width

            // 标题
            Rectangle {
                width: parent.width
                height: 60
                color: "#1e1e1e"

                Text {
                    text: "设置"
                    color: "#ffffff"
                    font.pixelSize: 18
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            // 设置分类列表
            ListView {
                width: parent.width
                height: parent.height - 60
                model: ListModel {
                    ListElement {
                        icon: "⚙"
                        name: "通用"
                        category: "general"
                    }
                    ListElement {
                        icon: "🎨"
                        name: "外观"
                        category: "appearance"
                    }
                    ListElement {
                        icon: "🔧"
                        name: "编辑器"
                        category: "editor"
                    }
                    ListElement {
                        icon: "🔌"
                        name: "扩展"
                        category: "extensions"
                    }
                    ListElement {
                        icon: "🌐"
                        name: "网络"
                        category: "network"
                    }
                    ListElement {
                        icon: "🔒"
                        name: "隐私"
                        category: "privacy"
                    }
                }
                delegate: Rectangle {
                    width: parent.width
                    height: 50
                    color: root.currentIndex === index ? "#094771" : "transparent"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        spacing: 12

                        Text {
                            text: icon
                            font.pixelSize: 18
                            color: "#cccccc"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: name
                            font.pixelSize: 14
                            color: "#cccccc"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.currentIndex = index
                            root.categorySelected(category)
                        }
                    }
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
