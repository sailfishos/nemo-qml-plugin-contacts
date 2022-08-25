TEMPLATE = aux

CONFIG += sailfish-qdoc-template
SAILFISH_QDOC.project = nemo-qml-plugin-contacts
SAILFISH_QDOC.config = nemo-qml-plugin-contacts.qdocconf
SAILFISH_QDOC.style = offline
SAILFISH_QDOC.path = /usr/share/doc/nemo-qml-plugin-contacts

OTHER_FILES += $$PWD/sailfish-contacts.qdocconf \
               $$PWD/index.qdoc
