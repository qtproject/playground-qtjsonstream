OTHER_FILES += $$PWD/jsonstream.qdocconf

docs_target.target = docs
docs_target.commands = qdoc $$PWD/jsonstream.qdocconf

QMAKE_EXTRA_TARGETS = docs_target
QMAKE_CLEAN += "-r $$PWD/html"
