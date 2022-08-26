/*
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

#include "seasideaddressbookutil.h"

#include <seasidecache.h>

#include <QtDebug>

/*!
  \qmltype AddressBookUtil
  \inqmlmodule org.nemomobile.contacts
*/
SeasideAddressBookUtil::SeasideAddressBookUtil(QObject *parent)
    : QObject(parent)
{
    const QList<QContactCollection> collections = SeasideCache::manager()->collections();
    for (const QContactCollection &collection : collections) {
        m_addressBooks.append(QVariant::fromValue(SeasideAddressBook::fromCollectionId(collection.id())));
    }

    connect(SeasideCache::manager(), &QContactManager::collectionsAdded,
            this, &SeasideAddressBookUtil::collectionsAdded);
    connect(SeasideCache::manager(), &QContactManager::collectionsRemoved,
            this, &SeasideAddressBookUtil::collectionsRemoved);
    connect(SeasideCache::manager(), &QContactManager::collectionsChanged,
            this, &SeasideAddressBookUtil::collectionsChanged);
}

SeasideAddressBookUtil::~SeasideAddressBookUtil()
{
}

/*!
  \qmlproperty array AddressBookUtil::addressBooks
*/
QVariantList SeasideAddressBookUtil::addressBooks() const
{
    return m_addressBooks;
}

void SeasideAddressBookUtil::collectionsAdded(const QList<QContactCollectionId> &collectionIds)
{
    for (const QContactCollectionId &id : collectionIds) {
        m_addressBooks.append(QVariant::fromValue(SeasideAddressBook::fromCollectionId(id)));
    }
    emit addressBooksChanged();
}

void SeasideAddressBookUtil::collectionsRemoved(const QList<QContactCollectionId> &collectionIds)
{
    for (const QContactCollectionId &id : collectionIds) {
        const int i = findCollection(id);
        if (i >= 0) {
            m_addressBooks.removeAt(i);
        }
    }
    emit addressBooksChanged();
}

void SeasideAddressBookUtil::collectionsChanged(const QList<QContactCollectionId> &collectionIds)
{
    bool emitChanged = false;
    for (const QContactCollectionId &id : collectionIds) {
        const int i = findCollection(id);
        if (i >= 0) {
            emitChanged = true;
            m_addressBooks.replace(i, QVariant::fromValue(SeasideAddressBook::fromCollectionId(id)));
        }
    }
    if (emitChanged) {
        emit addressBooksChanged();
    }
}

int SeasideAddressBookUtil::findCollection(const QContactCollectionId &id) const
{
    for (int i = 0; i < m_addressBooks.count(); ++i) {
        if (m_addressBooks.at(i).value<SeasideAddressBook>().collectionId == id) {
            return i;
        }
    }
    return -1;
}
