TEMPLATE = subdirs
SUBDIRS = src tools tests translations

tests.depends = src

OTHER_FILES += rpm/nemo-qml-plugin-contacts-qt5.spec
