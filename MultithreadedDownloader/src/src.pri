INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/mainwidget.cpp

HEADERS += \
    $$PWD/mainwidget.h

FORMS += \
    $$PWD/mainwidget.ui

include($$PWD/downloader/downloader.pri)
include($$PWD/customWidgets/customWidgets.pri)
