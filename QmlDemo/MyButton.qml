import QtQuick 2.15
import QtQuick.Controls 2.12

Button {
    id: myButton

    property color normalColor: "#E0E0E0"
    property color pressColor: "#CFCFCF"
    property color hoverColor: "#DBDBDB"

    property int clickedX: 0
    property int clickedY: 0

    hoverEnabled: true

    onPressed: {
        clickedX = pressX;
        clickedY = pressY;

        pressAnimation.restart();
    }

    onReleased: {
        if(pressAnimation.paused)
            pressAnimation.resume();
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40

        color: myButton.hovered ? hoverColor : normalColor
        clip: true

        Behavior on color {
            ColorAnimation { duration: 200 }
        }

        Rectangle {
            id: pressRect

            width: 0
            height: width

            x: clickedX - width / 2
            y: clickedY - width / 2

            color: pressColor
            radius: 180
        }

        SequentialAnimation {
            id: pressAnimation

            SmoothedAnimation {
                target: pressRect
                property: "width"
                from: 0
                to: Math.max(width, height) * 2
                duration: 800
            }

            ScriptAction {
                script: {
                    if(myButton.down)
                        pressAnimation.pause();
                }
            }

            OpacityAnimator {
                target: pressRect
                from: 1
                to: 0
                duration: 300
            }

            onFinished: {
                pressRect.width = 0;
                pressRect.opacity = 1;
            }
        }
    }
}
