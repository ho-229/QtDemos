import QtQuick 2.0
import QtQuick.Controls 2.12

Slider {
    id: slider

    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2

        implicitWidth: 200
        implicitHeight: 4

        width: slider.availableWidth
        height: implicitHeight

        radius: 2
        color: "#bdbebf"

        Rectangle {
            width: slider.visualPosition * parent.width
            height: parent.height
            color: "#3F85FF"
            radius: 2
        }
    }

    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: 16
        implicitHeight: 16
        radius: 13
        color: slider.pressed ? "#f0f0f0" : "#f6f6f6"
        border.color: "#bdbebf"
    }
}
