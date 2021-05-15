import QtQuick 2.15
import QtQuick.Window 2.15
import Qt.labs.platform 1.1
import QtQuick.Controls 2.15

import com.multimedia.videoplayer 1.0

Window {
    id: root

    width: 1280
    height: 720
    visible: true
    title: qsTr("Video Player")

    property bool fullScreen: false

    visibility: fullScreen ? Window.FullScreen : Window.Windowed

    VideoPlayer {
        id: videoPlayer
        anchors.fill: parent

        onSourceChanged: urlTitle.toast()

        onPositionChanged: playSlider.value = position
    }

    MouseArea {
        anchors.fill: parent

        hoverEnabled: true

        onPositionChanged: {
            urlTitle.toast();
            controlBar.toast();
        }

        onClicked: {
            urlTitle.toast();
            controlBar.toast();
        }

        onDoubleClicked: playBtn.clicked();
    }

    Toast {
        id: urlTitle

        height: 90

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        gradient:Gradient {
            GradientStop { position: 0.0; color: "black" }
            GradientStop { position: 1.0; color: "transparent" }
        }

        Text {
            text: videoPlayer.source

            font.pointSize: 15

            color: "white"

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 9
        }
    }

    Toast {
        id: controlBar

        height: 80

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        gradient:Gradient {
            GradientStop { position: 1.0; color: "black" }
            GradientStop { position: 0.0; color: "transparent" }
        }

        Button {
            id: playBtn

            width: 45
            height: 45

            anchors.centerIn: parent

            icon.source: videoPlayer.playing && !videoPlayer.paused ?
                             "qrc:/image/pause.png" : "qrc:/image/play.png"
            icon.color: "white"
            icon.width: 20
            icon.height: 20

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: {
                if(!videoPlayer.playing)
                {
                    videoPlayer.playing = true;
                    return;
                }

                videoPlayer.paused = !videoPlayer.paused
            }
        }

        Button {
            id: goaheadBtn

            width: 37
            height: 37

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: playBtn.right
            anchors.margins: 9

            icon.width: 15
            icon.height: 15
            icon.source: "qrc:/image/goahead.png"
            icon.color: "white"

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: {
                if(videoPlayer.playing)
                    videoPlayer.seek(videoPlayer.position + 10);
            }
        }

        Button {
            id: backBtn

            width: 37
            height: 37

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: playBtn.left
            anchors.margins: 9

            icon.width: 15
            icon.height: 15
            icon.source: "qrc:/image/back.png"
            icon.color: "white"

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: {
                if(videoPlayer.playing)
                    videoPlayer.seek(videoPlayer.position - 10);
            }
        }

        Button {
            id: fullScreenBtn

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 20

            background: Rectangle { color: "transparent" }

            icon.color: "white"
            icon.source: root.fullScreen ? "qrc:/image/exit_full_screen.png"
                                    : "qrc:/image/full_screen.png"

            onClicked: root.fullScreen = !root.fullScreen;
        }

        PlaySlider {
            id: playSlider

            y: 0 - height / 2

            from: 0
            to: videoPlayer.duration

            value: 0

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 9

            onPressedChanged: {
                if(!pressed)
                    videoPlayer.seek(value);
            }

            live: false
        }
    }

    Button {
        id: openBtn

        text: qsTr("<font color='white'>Open file</font>")
        font.pointSize: 16

        anchors.centerIn: parent

        visible: !videoPlayer.playing

        background: Rectangle {
            radius: 7
            color: "#3F85FF"
        }

        icon.source: "qrc:/image/file.png"
        icon.color: "white"

        onClicked: openFileDialog.open();
    }

    FileDialog {
        id: openFileDialog
        title: "Please choose a Video file"

        nameFilters: [ "Video files (*.mp4 *mkv)", "YUV files(*.yuv)" ]

        onAccepted: {
            videoPlayer.source = file;
            videoPlayer.playing = true;
        }
    }

    onClosing: videoPlayer.playing = false;
}
