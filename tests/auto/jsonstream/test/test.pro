CONFIG += testcase
CONFIG -= app_bundle

QT = jsonstream testlib

include(../../../../src/src.pri)

SOURCES = ../tst_jsonstream.cpp
TARGET = ../tst_jsonstream
