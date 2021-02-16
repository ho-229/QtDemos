import QtQuick 2.0
import QtQuick.Controls 2.12

Button {
    property color baseColor: "#E0E0E0"
    property color pressColor: "#CFCFCF"
    property color hoverColor: "#DBDBDB"

    hoverEnabled: true

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40

        color: parent.hovered ? (parent.down ? pressColor : hoverColor) : baseColor

        Behavior on color {
            ColorAnimation { duration: 200 }
        }
    }
}
