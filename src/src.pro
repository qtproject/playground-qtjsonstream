TEMPLATE = lib
TARGET   = $$QT.jsonstream.name
MODULE   = jsonstream

load(qt_module)
load(qt_module_config)

DESTDIR = $$QT.jsonstream.libs
VERSION = $$QT.jsonstream.VERSION
DEFINES += QT_ADDON_JSONSTREAM_LIB

QT += core network declarative

CONFIG += module create_prl
MODULE_PRI = ../modules/qt_jsonstream.pri

HEADERS += qtaddonjsonstreamversion.h

unix:!mac {
  PUBLIC_HEADERS += $$PWD/jsonpidauthority.h
  SOURCES += $$PWD/jsonpidauthority.cpp
}

BSON_HEADERS = \
    $$PWD/bson/bson_p.h \
    $$PWD/bson/platform_hacks_p.h \
    $$PWD/bson/qt-bson_p.h

SCHEMA_PUBLIC_HEADERS = \
    $$PWD/qtjsonschema/jsonschema-global.h \
    $$PWD/qtjsonschema/schemavalidator.h

SCHEMA_HEADERS = \
    $$PWD/qtjsonschema/schemaobject_p.h \
    $$PWD/qtjsonschema/checkpoints_p.h \
    $$PWD/qtjsonschema/schemamanager_impl_p.h \
    $$PWD/qtjsonschema/schemamanager_p.h \
    $$PWD/qtjsonschema/jsonobjecttypes_p.h \
    $$PWD/qtjsonschema/jsonobjecttypes_impl_p.h \
    $$PWD/qtjsonschema/varobjecttypes_p.h \
    $$PWD/qtjsonschema/varobjecttypes_impl_p.h

SCHEMA_SOURCES = \
    $$PWD/qtjsonschema/schemavalidator.cpp

PUBLIC_HEADERS += \
    $$PWD/jsonstream.h \
    $$PWD/jsonclient.h \
    $$PWD/jsonauthority.h \
    $$PWD/jsontokenauthority.h \
    $$PWD/jsonuidauthority.h \
    $$PWD/jsonserver.h \
    $$PWD/jsonstream-global.h \
    $$PWD/jsonserverclient.h \
    $$SCHEMA_PUBLIC_HEADERS

HEADERS += \
   $$PWD/jsonbuffer_p.h \
   $$BSON_HEADERS \
   $$PUBLIC_HEADERS \
   $$SCHEMA_HEADERS

SOURCES += \
    $$PWD/jsonstream.cpp \
    $$PWD/jsonbuffer.cpp \
    $$PWD/bson/bson.cpp \
    $$PWD/bson/qt-bson.cpp \
    $$PWD/bson/bson-scriptvalue.cpp \
    $$PWD/jsonclient.cpp \
    $$PWD/jsonauthority.cpp \
    $$PWD/jsontokenauthority.cpp \
    $$PWD/jsonuidauthority.cpp \
    $$PWD/jsonserverclient.cpp \
    $$PWD/jsonserver.cpp \
    $$SCHEMA_SOURCES

mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$QT.jsonstream.name
