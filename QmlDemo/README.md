# QmlDemo
* Just a QML Demo.
## Items
* [ClickWaveEffect](./src/customItem/clickwaveeffect.h)
    * Add [ClickWaveEffect](./src/customItem/clickwaveeffect.h) class to your project.
    * Example
        ```cpp
        // main.cpp
        ...
        #include "clickwaveeffect"

        int main(int argc, char*[] argv)
        {
            ...
            qmlRegisterType<ClickWaveEffect>("com.MyItems.Effect", 1, 0, "ClickWaveEffect");
            ...
        } 
        ```
        ```qml
        // main.qml
        import QtQuick 2.12
        import com.MyItems.Effect 1.0

        Rectangle {
            id: root

            ClickWaveEffect {
                target: root

                anchors.fill: parent
            }
        }
        ```