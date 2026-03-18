import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    implicitWidth: 480
    implicitHeight: 400

    property string softwareVersion: "1.0.0"
    property string hardwareVersion: "1.0.0"

    property var channelSoftwareVersions: (function () {
        var arr = []
        for (var i = 0; i < 36; i++) {
            arr.push("0.0.0.0")
        }
        return arr
    })()

    property var channelHardwareVersions: (function () {
        var arr = []
        for (var i = 0; i < 36; i++) {
            arr.push("0.0.0.0")
        }
        return arr
    })()

    //property real scaleFactor: 1.0
    property real scaleFactor: {
        if (width > 0 && height > 0) {
            return Math.min(width / implicitWidth, height / implicitHeight)
        }
        return 1.0
    }
    readonly property color bgColor: "#0d1b2a"
    readonly property color headerBgColor: "#2a3b4c"
    readonly property color textColor: "#e0e0e0"
    readonly property color accentColor: "#4a9eff"
    readonly property color borderColor: "#2a3b4c"

    Rectangle {
        anchors.fill: parent
        color: root.bgColor
        border.color: root.borderColor
        border.width: 1 * root.scaleFactor
        radius: 6 * root.scaleFactor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 2.4 * root.scaleFactor
            spacing: 2.4 * root.scaleFactor

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 8
                color: root.headerBgColor
                radius: 6 * root.scaleFactor
                border.color: root.borderColor
                border.width: 1 * root.scaleFactor

                RowLayout {
                    anchors.fill: parent

                    Text {
                        text: "SW-Ver:    V" + root.softwareVersion
                        font.pixelSize: 16 * root.scaleFactor
                        color: root.textColor
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "HW-Ver:    V" + root.hardwareVersion
                        font.pixelSize: 16 * root.scaleFactor
                        color: root.textColor
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            GridLayout {
                id: channelsGridinfrom
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 92
                columnSpacing: 1 * root.scaleFactor
                rowSpacing: 1 * root.scaleFactor
                columns: 9
                rows: 4

                Repeater {
                    model: 36 // channel count
                    delegate: Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.headerBgColor
                        radius: 6 * root.scaleFactor
                        border.color: root.borderColor
                        border.width: 1 * root.scaleFactor

                        ColumnLayout {
                            anchors.fill: parent

                            Text {
                                text: "CH_" + (index + 1)
                                font.pixelSize: 9.8 * root.scaleFactor
                                color: root.accentColor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Text {
                                text: "S:V" + root.channelSoftwareVersions[index]
                                font.pixelSize: 9 * root.scaleFactor
                                color: root.textColor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Text {
                                text: "H:V" + root.channelHardwareVersions[index]
                                font.pixelSize: 9 * root.scaleFactor
                                color: root.textColor
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }
    }

    function setAllChannelSoftwareVersions(versions) {
        if (versions && versions.length === 36) {
            root.channelSoftwareVersions = versions.slice()
        }
    }

    function setAllChannelHardwareVersions(versions) {
        if (versions && versions.length === 36) {
            root.channelHardwareVersions = versions.slice()
        }
    }
}
