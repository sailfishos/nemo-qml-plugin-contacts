include(../config.pri)

CONFIG += qt hide_symbols
CONFIG += link_pkgconfig

packagesExist(mlite$${QT_MAJOR_VERSION}) {
    PKGCONFIG += mlite$${QT_MAJOR_VERSION}
    DEFINES += HAS_MLITE
} else {
    packagesExist(gsettings-qt) {
        message("use of gsettings")
        PKGCONFIG += gsettings-qt
        DEFINES += HAS_QGSETTINGS
    } else {
        warning("Neither mlite nor gsettings available. Some functionality may not work as expected.")
    }
}

packagesExist(mce) {
    PKGCONFIG += mce
    DEFINES += HAS_MCE
} else {
    warning("mce not available. Some functionality may not work as expected.")
}

PKGCONFIG += mlocale$${QT_MAJOR_VERSION}
LIBS += -lphonenumber


# We need access to QtContacts private headers
QT += contacts-private

# We need the moc output for ContactManagerEngine from sqlite-extensions
extensionsIncludePath = $$system(pkg-config --cflags-only-I qtcontacts-sqlite-qt$${QT_MAJOR_VERSION}-extensions)
VPATH += $$replace(extensionsIncludePath, -I, )
HEADERS += \
    contactmanagerengine.h \
    qcontactclearchangeflagsrequest.h

SOURCES += \
    $$PWD/cacheconfiguration.cpp \
    $$PWD/seasidecache.cpp \
    $$PWD/seasideexport.cpp \
    $$PWD/seasideimport.cpp \
    $$PWD/seasidecontactbuilder.cpp \
    $$PWD/seasidepropertyhandler.cpp

PUBLIC_HEADERS = \
    $$PWD/cacheconfiguration.h \
    $$PWD/contactcacheexport.h \
    $$PWD/seasidecache.h \
    $$PWD/seasideexport.h \
    $$PWD/seasideimport.h \
    $$PWD/seasidecontactbuilder.h \
    $$PWD/synchronizelists.h \
    $$PWD/seasidepropertyhandler.h

HEADERS += $$PUBLIC_HEADERS

