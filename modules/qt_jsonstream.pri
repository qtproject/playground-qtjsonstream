QT.jsonstream.VERSION = 1.0.0
QT.jsonstream.MAJOR_VERSION = 1
QT.jsonstream.MINOR_VERSION = 0
QT.jsonstream.PATCH_VERSION = 0

QT.jsonstream.name = QtAddOnJsonStream
QT.jsonstream.bins = $$QT_MODULE_BIN_BASE
QT.jsonstream.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtAddOnJsonStream
QT.jsonstream.private_includes = $$QT_MODULE_INCLUDE_BASE/QtAddOnJsonStream/$$QT.jsonstream.VERSION
QT.jsonstream.sources = $$QT_MODULE_BASE/src
QT.jsonstream.libs = $$QT_MODULE_LIB_BASE
QT.jsonstream.plugins = $$QT_MODULE_PLUGIN_BASE
QT.jsonstream.imports = $$QT_MODULE_IMPORT_BASE
QT.jsonstream.depends = core network

QT_CONFIG += jsonstream
