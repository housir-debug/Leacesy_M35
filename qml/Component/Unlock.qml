// StyledButton.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Button {
    id: control

    // 自定义属性
    property string buttonText: "Swipe to confirm"
    property color primaryColor: "#d4dbe0"
    property color accentColor: "#6a7175"
    property bool confirmed: false

    // 信号
    signal confirmed

    width: 320
    height: 75

    background: Rectangle {
        radius: 200
        color: control.down ? Qt.darker(primaryColor, 1.05) : primaryColor
        border.width: 0

        // 外层阴影
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 15
            radius: 30
            samples: 31
            color: "#20000000"
        }

        // 内阴影效果（模拟 CSS box-shadow inset）
        Rectangle {
            anchors.fill: parent
            radius: 200
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: "#45204030"
                }
                GradientStop {
                    position: 0.5
                    color: "#00ffffff"
                }
                GradientStop {
                    position: 1.0
                    color: "#30ffffff"
                }
            }
            opacity: 0.5
        }

        // 悬停效果
        Behavior on color {
            ColorAnimation {
                duration: 500
            }
        }
    }

    contentItem: Item {
        // 滑动按钮（圆形滑块）
        Rectangle {
            id: slider
            width: 66.7
            height: 66.7
            radius: 33.35
            anchors.verticalCenter: parent.verticalCenter
            x: control.confirmed ? parent.width - width - 10 : 5

            color: "#33ffffff"
            opacity: 0.9

            // 内阴影效果
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.width: 1
                border.color: "#40ffffff"
            }

            // 锁图标
            Image {
                anchors.centerIn: parent
                width: 29
                height: 29
                source: "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1024 1024'><path fill='%23899298' d='M800 960H224c-52.8 0-96-43.2-96-96V480c0-52.8 43.2-96 96-96h576c52.8 0 96 43.2 96 96v384c0 52.8-43.2 96-96 96zM224 448c-17.6 0-32 14.4-32 32v384c0 17.6 14.4 32 32 32h576c17.6 0 32-14.4 32-32V480c0-17.6-14.4-32-32-32H224z m528-32c-17.6 0-32-14.4-32-32V272c0-115.2-92.8-208-208-208s-208 92.8-208 208v112c0 17.6-14.4 32-32 32s-32-14.4-32-32V272C240 121.6 361.6 0 512 0s272 121.6 272 272v112c0 17.6-14.4 32-32 32z'/></svg>"
                fillMode: Image.PreserveAspectFit

                Behavior on rotation {
                    RotationAnimation {
                        duration: 500
                    }
                }
            }

            Behavior on x {
                NumberAnimation {
                    duration: 800
                    easing.type: Easing.InOutCubic
                }
            }
        }

        // 文字容器
        Item {
            anchors.centerIn: parent
            x: control.confirmed ? parent.width * 0.5 : parent.width * 0.57

            Behavior on x {
                NumberAnimation {
                    duration: 800
                    easing.type: Easing.InOutCubic
                }
            }

            Row {
                spacing: 0
                anchors.centerIn: parent

                Repeater {
                    model: control.buttonText.split('')

                    Text {
                        text: modelData
                        color: control.confirmed ? "#505050" : "#8b9398"
                        font.pixelSize: 20
                        font.family: "Segoe UI"
                        font.weight: Font.DemiBold
                        //letterSpacing: -2.7

                        // 悬停动画
                        SequentialAnimation {
                            running: !control.confirmed && control.hovered
                            loops: Animation.Infinite

                            NumberAnimation {
                                target: parent
                                property: "rotation"
                                to: 7
                                duration: 1150
                                easing.type: Easing.InOutQuad
                            }
                            NumberAnimation {
                                target: parent
                                property: "rotation"
                                to: -7
                                duration: 1150
                                easing.type: Easing.InOutQuad
                            }
                        }

                        Behavior on color {
                            ColorAnimation {
                                duration: 500
                            }
                        }
                    }
                }
            }
        }

        // 完成文字
        Text {
            text: "Done"
            anchors.centerIn: parent
            color: "#808080"
            font.pixelSize: 20
            font.family: "Segoe UI"
            font.weight: Font.Bold
            opacity: control.confirmed ? 0.8 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 460
                    easing.type: Easing.OutCubic
                }
            }
        }

        // 背景装饰 - 波浪效果
        Item {
            anchors.fill: parent
            visible: !control.confirmed

            Rectangle {
                width: 260
                height: 260
                radius: 110
                color: "#0a00569f"
                opacity: 0.04
                x: -100
                y: parent.height + 50

                RotationAnimation on rotation {
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 15000
                }
            }

            Rectangle {
                width: 280
                height: 280
                radius: 130
                color: "#0a00569f"
                opacity: 0.04
                x: parent.width * 0.4
                y: parent.height + 60

                RotationAnimation on rotation {
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 13000
                }
            }

            Rectangle {
                width: 350
                height: 350
                radius: 140
                color: "#0a00569f"
                opacity: 0.04
                x: -150
                y: parent.height + 100

                RotationAnimation on rotation {
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 30000
                }
            }
        }

        // 月亮图标
        Image {
            source: "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1024 1024'><path fill='%2310335f17' d='M518.8 512.7c0-178.9 116.1-330.9 278.5-389.1-45.6-16.3-94.6-25.7-145.9-25.7C417 97.9 227 283.7 227 512.7c0 229.1 190 414.8 424.5 414.8 51.4 0 100.3-9.4 145.9-25.7-162.5-58.1-278.6-210.1-278.6-389.1z'/></svg>"
            width: 22
            height: 22
            anchors.right: parent.right
            anchors.rightMargin: 34
            anchors.top: parent.top
            anchors.topMargin: 13
            opacity: control.hovered ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 400
                }
            }

            SequentialAnimation on rotation {
                running: control.hovered
                loops: Animation.Infinite
                alwaysRunToEnd: true

                NumberAnimation {
                    to: -190
                    duration: 1700
                    easing.type: Easing.InOutSine
                }
                NumberAnimation {
                    to: -210
                    duration: 1700
                    easing.type: Easing.InOutSine
                }
            }
        }
    }

    // 点击确认逻辑
    onClicked: {
        if (!confirmed) {
            confirmed = true
            confirmed()
        }
    }

    // 重置功能（外部调用）
    function reset() {
        confirmed = false
    }
}
