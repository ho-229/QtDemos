INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/main.cpp

include($$PWD/player/player.pri)
include($$_PRO_FILE_PWD_/src/until/until.pri)
