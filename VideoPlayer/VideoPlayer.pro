QT += quick

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
    # FFmpeg
    LIBS += -L$$(FFMPEG_PATH)/lib/ -lavutil -lavcodec -lavformat -lavfilter -lswresample -lswscale
    INCLUDEPATH += $$(FFMPEG_PATH)/include
    DEPENDPATH += $$(FFMPEG_PATH)/include

    # SDL2
    LIBS += -L$$(SDL_PATH)/lib/x64/ -lSDL2
    INCLUDEPATH += $$(SDL_PATH)/include
    DEPENDPATH += $$(SDL_PATH)/include
}

unix {
    LIBS += -L/usr/lib/ -lavutil -lavcodec -lavformat -lavfilter -lswresample -lswscale
    INCLUDEPATH += /usr/include
    DEPENDPATH += /usr/include
}

win32:
