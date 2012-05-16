TARGET = QtAddOnJsonStream
QT += core network
QT -= gui

QMAKE_DOCS = $$PWD/doc/jsonstream.qdocconf

load(qt_module)

DEFINES += QT_ADDON_JSONSTREAM_LIB

unix:!mac {
  PUBLIC_HEADERS += $$PWD/qjsonpidauthority.h
  SOURCES += $$PWD/qjsonpidauthority.cpp
}

BSON_HEADERS = \
    $$PWD/bson/bson_p.h \
    $$PWD/bson/platform_hacks_p.h \
    $$PWD/bson/qt-bson_p.h

SCHEMA_PUBLIC_HEADERS = \
    $$PWD/qtjsonschema/qjsonschema-global.h \
    $$PWD/qtjsonschema/qjsonschemaerror.h \
    $$PWD/qtjsonschema/qjsonschemavalidator.h

SCHEMA_HEADERS = \
    $$PWD/qtjsonschema/schemaobject_p.h \
    $$PWD/qtjsonschema/checkpoints_p.h \
    $$PWD/qtjsonschema/schemamanager_impl_p.h \
    $$PWD/qtjsonschema/schemamanager_p.h \
    $$PWD/qtjsonschema/jsonobjecttypes_p.h \
    $$PWD/qtjsonschema/jsonobjecttypes_impl_p.h

SCHEMA_SOURCES = \
    $$PWD/qtjsonschema/qjsonschemaerror.cpp \
    $$PWD/qtjsonschema/qjsonschemavalidator.cpp

PUBLIC_HEADERS += \
    $$PWD/qjsonstream.h \
    $$PWD/qjsonclient.h \
    $$PWD/qjsonauthority.h \
    $$PWD/qjsontokenauthority.h \
    $$PWD/qjsonuidauthority.h \
    $$PWD/qjsonuidrangeauthority.h \
    $$PWD/qjsonserver.h \
    $$PWD/qjsonstream-global.h \
    $$PWD/qjsonserverclient.h \
    $$PWD/qjsonpipe.h \
    $$PWD/qjsonconnection.h \
    $$PWD/qjsonendpoint.h \
    $$SCHEMA_PUBLIC_HEADERS

HEADERS += \
   $$PWD/qjsonbuffer_p.h \
   $$PWD/qjsonconnectionprocessor_p.h \
   $$PWD/qjsonendpointmanager_p.h \
   $$BSON_HEADERS \
   $$PUBLIC_HEADERS \
   $$SCHEMA_HEADERS

SOURCES += \
    $$PWD/qjsonstream.cpp \
    $$PWD/qjsonbuffer.cpp \
    $$PWD/bson/bson.cpp \
    $$PWD/bson/qt-bson.cpp \
    $$PWD/qjsonclient.cpp \
    $$PWD/qjsonauthority.cpp \
    $$PWD/qjsontokenauthority.cpp \
    $$PWD/qjsonuidauthority.cpp \
    $$PWD/qjsonuidrangeauthority.cpp \
    $$PWD/qjsonserverclient.cpp \
    $$PWD/qjsonserver.cpp \
    $$PWD/qjsonpipe.cpp \
    $$PWD/qjsonconnection.cpp \
    $$PWD/qjsonconnectionprocessor.cpp \
    $$PWD/qjsonendpoint.cpp \
    $$PWD/qjsonendpointmanager.cpp \
    $$SCHEMA_SOURCES \

mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
