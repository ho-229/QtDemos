import QtQuick 2.15

Rectangle {
    id: myToast

    visible: false

    property bool closeAvailable: true

    function toast() {
        if(showAnimation.running)
            return;
        else if(pauseTimer.running)
        {
            pauseTimer.restart();
            return;
        }
        else if(hideAnimation.running)
            hideAnimation.stop();

        opacity = 0;
        visible = true;
        showAnimation.start();
    }

    OpacityAnimator {
        id: showAnimation

        target: myToast;
        from: 0
        to: 1
        duration: 300

        onFinished: pauseTimer.start();
    }

    OpacityAnimator {
        id: hideAnimation

        target: myToast
        from: 1
        to: 0
        duration: 300

        onFinished: visible = false
    }

    Timer {
        id: pauseTimer

        interval: 3000

        onTriggered: {
            if(!closeAvailable)
                pauseTimer.restart();
            else
                hideAnimation.start()
        }
    }
}
