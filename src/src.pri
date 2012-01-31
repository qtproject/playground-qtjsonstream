INCLUDEPATH += $$PWD

QT += network declarative

mac|unix {
    CONFIG += rpath_libdirs
    QMAKE_RPATHDIR += $$OUT_PWD/../lib
    QMAKE_LFLAGS += "-Wl,-rpath $$PWD"
}
