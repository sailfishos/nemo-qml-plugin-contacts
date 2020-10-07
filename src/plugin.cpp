/*
 * Copyright (c) 2012 Robin Burchell <robin+nemo@viroteck.net>
 * Copyright (c) 2012 – 2020 Jolla Ltd.
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include <QtGlobal>
#include <QTranslator>

#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include "seasideaddressbook.h"
#include "seasideaddressbookmodel.h"
#include "seasideperson.h"
#include "seasidefilteredmodel.h"
#include "seasidedisplaylabelgroupmodel.h"
#include "seasidevcardmodel.h"
#include "seasideconstituentmodel.h"
#include "seasidemergecandidatemodel.h"
#include "knowncontacts.h"

template <typename T> static QObject *singletonApiCallback(QQmlEngine *engine, QJSEngine *) {
    return new T(engine);
}

class AppTranslator: public QTranslator
{
    Q_OBJECT
public:
    AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    virtual ~AppTranslator()
    {
        qApp->removeTranslator(this);
    }
};

class Q_DECL_EXPORT NemoContactsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.nemomobile.contacts")

public:
    virtual ~NemoContactsPlugin() { }

    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("org.nemomobile.contacts"));

        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        AppTranslator *translator = new AppTranslator(engine);
        engineeringEnglish->load("nemo-qml-plugin-contacts_eng_en", "/usr/share/translations");
        translator->load(QLocale(), "nemo-qml-plugin-contacts", "-", "/usr/share/translations");
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("org.nemomobile.contacts"));

        qmlRegisterType<SeasideFilteredModel>(uri, 1, 0, "PeopleModel");
        qmlRegisterType<SeasideAddressBookModel>(uri, 1, 0, "AddressBookModel");
        qmlRegisterType<SeasideDisplayLabelGroupModel>(uri, 1, 0, "PeopleDisplayLabelGroupModel");
        qmlRegisterType<SeasidePersonAttached>();
        qmlRegisterType<SeasidePerson>(uri, 1, 0, "Person");
        qmlRegisterType<SeasideVCardModel>(uri, 1, 0, "PeopleVCardModel");
        qmlRegisterType<SeasideConstituentModel>(uri, 1, 0, "ConstituentModel");
        qmlRegisterType<SeasideMergeCandidateModel>(uri, 1, 0, "MergeCandidateModel");
        qmlRegisterUncreatableType<SeasideAddressBook>(uri, 1, 0, "AddressBook", "");
        qmlRegisterSingletonType<KnownContacts>(uri, 1, 0, "KnownContacts", singletonApiCallback<KnownContacts>);
    }
};

#include "plugin.moc"
