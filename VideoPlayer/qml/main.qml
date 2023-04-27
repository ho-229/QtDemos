import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.15

import com.multimedia.videoplayer 1.0

Window {
    id: root

    width: 1280
    height: 720
    minimumWidth: 700
    minimumHeight: 450

    visible: true
    title: qsTr("Video Player")

    property bool fullScreen: false

    visibility: fullScreen ? Window.FullScreen : Window.Windowed

    function onPlay() { playBtn.clicked() }
    function onBack() { backBtn.clicked() }
    function onGoahead() { goAheadBtn.clicked() }
    function onEscape() { fullScreen = false; }

    function prefixZero(num, n) {
        return (Array(n).join(0) + num).slice(-n);
    }

    function toMMSS(secs) {
        return parseInt(secs / 60) + ":" + prefixZero(secs % 60, 2);
    }

    MessageDialog {
        id: errorDialog

        title: qsTr("Error")
        text: videoPlayer.errorString
    }

    VideoPlayer {
        id: videoPlayer
        anchors.fill: parent

        volume: volumeSlider.value

        onSourceChanged: urlTitle.toast()

        onErrorOccurred: errorDialog.visible = true
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

            icon.source: videoPlayer.playbackState == VideoPlayer.Playing ?
                             "qrc:/image/pause.png" : "qrc:/image/play.png"
            icon.color: "white"
            icon.width: 20
            icon.height: 20

            focusPolicy: Qt.NoFocus

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: {
                if(videoPlayer.playbackState == VideoPlayer.Playing)
                    videoPlayer.pause();
                else
                    videoPlayer.play();
            }
        }

        Button {
            id: goAheadBtn

            width: 37
            height: 37

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: playBtn.right
            anchors.margins: 9

            icon.width: 15
            icon.height: 15
            icon.source: "qrc:/image/goahead.png"
            icon.color: "white"

            focusPolicy: Qt.NoFocus

            enabled: videoPlayer.playbackState == VideoPlayer.Playing && videoPlayer.seekable

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: videoPlayer.seek(videoPlayer.position + 10);
        }

        Button {
            id: volumeBtn

            property bool hasVolume: true
            property real oldVolume

            width: 37
            height: 37

            anchors.left: goAheadBtn.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 18

            icon.source: hasVolume ? "qrc:/image/volume.png" : "qrc:/image/non-volume.png"
            icon.color: "white"
            icon.width: 26
            icon.height: 26

            focusPolicy: Qt.NoFocus

            background: Rectangle { color: "transparent" }

            onClicked: hasVolume = !hasVolume

            onHasVolumeChanged: {
                if(hasVolume)
                    volumeSlider.value = oldVolume;
                else
                {
                    oldVolume = volumeSlider.value;
                    volumeSlider.value = 0;
                }
            }
        }

        PlaySlider {
            id: volumeSlider

            width: 90

            anchors.left: volumeBtn.right
            anchors.verticalCenter: parent.verticalCenter

            focusPolicy: Qt.NoFocus

            value: 1

            ToolTip {
                parent: volumeSlider.handle
                visible: volumeSlider.pressed
                text: qsTr("Volume: ") + parseInt(volumeSlider.value * 100)
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

            focusPolicy: Qt.NoFocus

            enabled: videoPlayer.playbackState == VideoPlayer.Playing && videoPlayer.seekable

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: videoPlayer.seek(videoPlayer.position - 10);
        }

        Button {
            id: stopBtn

            width: 29
            height: 29

            enabled: videoPlayer.playbackState != VideoPlayer.Stopped

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: backBtn.left
            anchors.rightMargin: 18

            icon.source: "qrc:/image/stop.png"
            icon.color: "white"
            icon.width: 11
            icon.height: 11

            focusPolicy: Qt.NoFocus

            background: Rectangle {
                color: "transparent"

                radius: 90

                border.width: 2
                border.color: "white"
            }

            onClicked: videoPlayer.stop()
        }

        Text {
            id: progressText

            property string durationString: toMMSS(videoPlayer.duration)

            text: toMMSS(videoPlayer.position) + " / " + durationString

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 28

            color: "white"

            font.pointSize: 12

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

            focusPolicy: Qt.NoFocus

            onClicked: root.fullScreen = !root.fullScreen;
        }

        PlaySlider {
            id: playSlider

            y: 0 - height / 2

            from: 0
            to: videoPlayer.duration

            value: videoPlayer.position

            enabled: videoPlayer.seekable && videoPlayer.playbackState != VideoPlayer.Stopped

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 9

            live: false

            onPressedChanged: {
                if(!pressed)
                    videoPlayer.seek(value);
            }

            ToolTip {
                parent: playSlider.handle
                visible: playSlider.hovered
                text: toMMSS(videoPlayer.position)
            }
        }

        Button {
            id: settingsBtn

            width: 37
            height: 37

            anchors.right: fullScreenBtn.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 21

            background: Rectangle { color: "transparent" }

            icon.source: "qrc:/image/settings.png"
            icon.color: "white"
            icon.width: 35
            icon.height: 35

            onClicked: settingsRect.toast()
        }

        Toast {
            id: settingsRect

            visible: false

            anchors.bottom: settingsBtn.top
            anchors.horizontalCenter: settingsBtn.horizontalCenter
            anchors.margins: 12

            height: 190
            width: 150

            color: "black"
            radius: 5

            Column {
                anchors.fill: parent
                padding: 6
                spacing: 6

                Text {
                    text: qsTr("Settings")
                    color: "white"

                    font.pointSize: 16
                }

                Text {
                    text: qsTr("Audio Track")
                    color: "white"

                    font.pointSize: 11
                }

                SpinBox {
                    from: videoPlayer.hasAudio ? 0 : -1
                    value: videoPlayer.activeAudioTrack
                    to: videoPlayer.audioTrackCount - 1

                    onValueChanged: {
                        if (videoPlayer.hasAudio)
                            videoPlayer.activeAudioTrack = value
                    }
                }

                Text {
                    text: qsTr("Subtitle Track")
                    color: "white"

                    font.pointSize: 11
                }

                SpinBox {
                    from: videoPlayer.hasSubtitle ? 0 : -1
                    value: videoPlayer.activeSubtitleTrack
                    to: videoPlayer.subtitleTrackCount - 1

                    onValueChanged: {
                        if (videoPlayer.hasSubtitle)
                            videoPlayer.activeSubtitleTrack = value
                    }
                }
            }

            HoverHandler { onHoveredChanged: parent.closeAvailable = !hovered }
        }

        HoverHandler { id: hoverHandler }

        closeAvailable: !hoverHandler.hovered && !settingsRect.visible
    }

    Button {
        id: openBtn

        text: qsTr("<font color='white'>Open file<br>or drop the link here</font>")
        font.pointSize: 14

        focusPolicy: Qt.NoFocus

        anchors.centerIn: parent

        visible: videoPlayer.playbackState == VideoPlayer.Stopped

        background: Rectangle {
            radius: 7
            color: "#3F85FF"
        }

        icon.source: "qrc:/image/file.png"
        icon.color: "white"
        icon.height: 50
        icon.width: 50

        onClicked: openFileDialog.open();

        DropArea {
            id: dropArea

            anchors.fill: parent

            onDropped: {
                if(drop.hasUrls || drop.hasText) {
                    openBtn.text = qsTr("<font color='white'>Loading...</font>")

                    videoPlayer.source = drop.hasUrls ? drop.urls[0] : drop.text;
                    videoPlayer.play();

                    openBtn.text
                            = qsTr("<font color='white'>Open file<br>Or drop here</font>")
                }
            }

            onEntered: openBtn.text
                       = qsTr("<font color='white'>Release to open file</font>");
            onExited: openBtn.text
                      = qsTr("<font color='white'>Open file<br>Or drop the link here</font>")
        }
    }

    FileDialog {
        id: openFileDialog
        title: "Please choose a Video file"

        nameFilters: [ "Video files (*.mp4 *.mkv *.flv)", "YUV files(*.yuv)" ]

        onAccepted: {
            videoPlayer.source = fileUrl;
            videoPlayer.play();
        }
    }

    onClosing: videoPlayer.stop();
}
