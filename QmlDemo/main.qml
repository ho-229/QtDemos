﻿import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import com.MyItems.Effect 1.0

Window {
    id: root
    width: 550
    height: 400
    visible: true
    title: qsTr("Hello World")

    minimumWidth: 400
    minimumHeight: 350

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        GroupBox {
            id:fontStyleGroup
            title: qsTr("Font Style")
            Layout.fillWidth: true
            Layout.margins: 9

            RowLayout {
                anchors.fill: parent

                CheckBox {
                    id: underlineCheck
                    text: qsTr("Underline")

                    onCheckedChanged: {
                        textEdit.font.underline = checkState == Qt.Checked
                    }
                }
                CheckBox {
                    id: italicCheck
                    text: qsTr("Italic")

                    onCheckStateChanged: {
                        textEdit.font.italic = checkState == Qt.Checked
                    }
                }
                CheckBox {
                    id: boldCheck
                    text: qsTr("Bold")

                    onCheckStateChanged: {
                        textEdit.font.bold = checkState == Qt.Checked
                    }
                }
            }
        }

        GroupBox {
            id: fontColorGroup
            title: qsTr("Color")
            Layout.fillWidth: true
            Layout.margins: 9

            RowLayout {
                anchors.fill: parent

                RadioButton {
                    id: blackRadio
                    text: qsTr("Black")
                    hoverEnabled: true
                    checked: true

                    onClicked: {
                        textEdit.color = "black"
                    }
                }
                RadioButton {
                    id: redRadio
                    text: qsTr("Red")

                    onClicked: {
                        textEdit.color = "red"
                    }
                }
                RadioButton {
                    id: blueRadio
                    text: qsTr("Blud")

                    onClicked: {
                        textEdit.color = "blue"
                    }
                }
            }
        }

        TextField {
            id: textEdit
            text: qsTr("Hello World")
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 9

            font.pointSize: 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            selectionColor: "#0000ff"

            selectByMouse: true
            readOnly: true

            background: ClickWaveEffect {
                target: textEdit

                maxRadius: 20
                waveDuration: 100

                anchors.fill: parent
            }
        }

        RowLayout {
            id: rowLayout
            Layout.preferredWidth: parent.width

            Button {
                id: myButton
                text: qsTr("Push Button")
                Layout.margins: 9
                Layout.alignment: Qt.AlignLeft //| Qt.AlignHCenter

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 40

                    color: "#E0E0E0"

                    ClickWaveEffect {
                        target: myButton

                        anchors.fill: parent
                    }
                }
            }

            Button {
                id: closeButton
                text: qsTr("Close")
                Layout.alignment: Qt.AlignRight //| Qt.AlignHCenter
                Layout.margins: 9

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 40

                    color: "#E0E0E0"

                    ClickWaveEffect {
                        target: closeButton

                        anchors.fill: parent
                    }
                }

                onClicked: root.close()
            }
        }
    }
}
