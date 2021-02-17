import QtQuick 2.15
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.12

Button {
    property color baseColor: "#E0E0E0"
    property color pressColor: "#CFCFCF"
    property color hoverColor: "#DBDBDB"

    property int clickedX: 0
    property int clickedY: 0

    property bool isWaveFinished: false

    hoverEnabled: true

    onPressed: {
        clickedX = pressX;
        clickedY = pressY;

        pressAnimation.restart();
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40

        color: parent.hovered ? (parent.down && isWaveFinished ? pressColor : hoverColor) : baseColor
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
                id: waveAnimation
                target: pressRect
                property: "width"
                from: 0
                to: Math.max(width, height) * 2
                duration: 800

                onRunningChanged: {
                    console.log("Running Changed");
                }
            }

            ScriptAction {
                script: isWaveFinished = true
            }

            OpacityAnimator {
                target: pressRect
                from: 1
                to: 0
                duration: 300
            }

            onStarted: isWaveFinished = false

            onFinished: {
                pressRect.width = 0;
                pressRect.opacity = 1;
            }
        }
    }
}
