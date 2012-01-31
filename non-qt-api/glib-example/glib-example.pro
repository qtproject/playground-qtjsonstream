QT       -= core gui

HEADERS += \
    glib-example.h \
    jsonclient.h \
    network.h

SOURCES += \
    glib-example.cpp \
    jsonclient.cpp \
    network.cpp

CONFIG += link_pkgconfig
PKGCONFIG += glib-2.0 sigc++-2.0 gio-unix-2.0 json-glib-1.0





