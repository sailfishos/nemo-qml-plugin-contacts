/*
 * Copyright (c) 2015 - 2020 Jolla Ltd.
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

#ifndef SEASIDECONTACTBUILDER_H
#define SEASIDECONTACTBUILDER_H

#include "contactcacheexport.h"

#include <QContact>
#include <QContactManager>
#include <QVersitDocument>
#include <QVersitContactHandler>

#include <QList>

QTCONTACTS_USE_NAMESPACE
QTVERSIT_USE_NAMESPACE

class CONTACTCACHE_EXPORT SeasideContactBuilderPrivate
{
public:
    QContactManager *manager;
    QVersitContactHandler *propertyHandler;

    QSet<QContactDetail::DetailType> unimportableDetailTypes;
    QStringList importableSyncTargets;

    QHash<QString, int> importGuids;
    QHash<QString, int> importNames;
    QHash<QString, int> importLabels;

    QHash<QString, QContactId> existingGuids;
    QHash<QString, QContactId> existingNames;
    QMap<QContactId, QString> existingContactNames;
    QHash<QString, QContactId> existingNicknames;

    QVariantMap extraData; // anything the derived type wants to store.
};

class CONTACTCACHE_EXPORT SeasideContactBuilder
{
public:
    SeasideContactBuilder();
    virtual ~SeasideContactBuilder();

    virtual QVersitContactHandler *propertyHandler();
    virtual QContactManager *manager();
    virtual QContactFilter mergeSubsetFilter() const;

    virtual bool mergeLocalIntoImport(QContact &import, const QContact &local, bool *erase);
    virtual bool mergeImportIntoImport(QContact &import, QContact &otherImport, bool *erase);

    virtual QList<QContact> importContacts(const QList<QVersitDocument> &documents);
    virtual void preprocessContact(QContact &contact);
    virtual int previousDuplicateIndex(QList<QContact> &importedContacts, int contactIndex);
    virtual void buildLocalDeviceContactIndexes();
    virtual QContactId matchingLocalContactId(QContact &contact);


protected:
    SeasideContactBuilderPrivate *d;
};

#endif
