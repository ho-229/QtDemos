QT += widgets quick multimedia

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RESOURCES += $$_PRO_FILE_PWD_/qml/qml.qrc \
   resource/res.qrc \
   shaders/shaders.qrc

TRANSLATIONS += \
    VideoPlayer_zh_CN.ts

include($$_PRO_FILE_PWD_/src/src.pri)

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32 {
    LIBS += -L$$PWD/ffmpeg/win64/lib/ -lavutil -lavcodec -lavformat -lswresample -lswscale
    INCLUDEPATH += $$PWD/ffmpeg/win64/include
    DEPENDPATH += $$PWD/ffmpeg/win64/include
}

unix {
    LIBS += -L/usr/lib/ -lavutil -lavcodec -lavformat -lswresample -lswscale
    INCLUDEPATH += /usr/include
    DEPENDPATH += /usr/include
}
