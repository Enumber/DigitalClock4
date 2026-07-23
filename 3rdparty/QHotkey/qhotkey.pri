# qmake include for QHotkey (upstream dropped qmake support, CMake only now).
# Builds QHotkey sources directly into the including target.
QT += gui

INCLUDEPATH += $$PWD/QHotkey
DEPENDPATH += $$PWD/QHotkey

HEADERS += \
    $$PWD/QHotkey/qhotkey.h \
    $$PWD/QHotkey/qhotkey_p.h

SOURCES += $$PWD/QHotkey/qhotkey.cpp

win32 {
    SOURCES += $$PWD/QHotkey/qhotkey_win.cpp
} else:macx {
    SOURCES += $$PWD/QHotkey/qhotkey_mac.cpp
    LIBS += -framework Carbon
} else:unix {
    SOURCES += $$PWD/QHotkey/qhotkey_x11.cpp
    lessThan(QT_MAJOR_VERSION, 6): QT += x11extras
    LIBS += -lX11
}
