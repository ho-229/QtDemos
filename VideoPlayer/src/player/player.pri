INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
   $$PWD/audiooutput.h \
   $$PWD/config.h \
   $$PWD/ffmpeg.h \
   $$PWD/ffmpegdecoder.h \
   $$PWD/videoplayer.h \
   $$PWD/videoplayer_p.h \
   $$PWD/videorenderer.h

SOURCES += \
   $$PWD/audiooutput.cpp \
   $$PWD/ffmpegdecoder.cpp \
   $$PWD/videoplayer.cpp \
   $$PWD/videoplayer_p.cpp \
   $$PWD/videorenderer.cpp
