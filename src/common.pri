QT += declarative

COMMON_PRIVATE_HEADERS = \
    $$PWD/bson/bson.h \
    $$PWD/bson/platform_hacks.h \
    $$PWD/bson/qt-bson.h

COMMON_PUBLIC_HEADERS = \
    $$PWD/jsonstream.h \
    $$PWD/jsonbuffer.h

COMMON_SOURCES = \
    $$PWD/jsonstream.cpp \
    $$PWD/jsonbuffer.cpp \
    $$PWD/bson/bson.cpp \
    $$PWD/bson/qt-bson.cpp \
    $$PWD/bson/bson-scriptvalue.cpp

INCLUDEPATH += $$PWD $$PWD/bson
