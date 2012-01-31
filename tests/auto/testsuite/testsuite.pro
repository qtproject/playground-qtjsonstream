CONFIG += testcase
CONFIG -= app_bundle

QT = core testlib network

include(../../../src/server/server.pri)
include(../../../src/client/client.pri)

HEADERS = tst_jsonclient.h tst_jsonserver.h
SOURCES = tst_jsonserver.cpp tst_jsonclient.cpp tst_jsonstream.cpp
TARGET = test_jsonstream_api
