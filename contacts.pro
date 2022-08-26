TEMPLATE = subdirs
SUBDIRS = lib src tools tests translations doc

src.depends = lib
tools.depends = src
tests.depends = src

OTHER_FILES += rpm/nemo-qml-plugin-contacts-qt5.spec
