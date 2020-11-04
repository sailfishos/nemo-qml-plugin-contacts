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

#include "seasideaddressbookmodel.h"

#include <seasidecache.h>

#include <QtDebug>
#include <QQmlInfo>

SeasideAddressBookModel::SeasideAddressBookModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const QList<QContactCollection> collections = SeasideCache::manager()->collections();
    for (const QContactCollection &collection : collections) {
        m_addressBooks.append(SeasideAddressBook::fromCollectionId(collection.id()));
    }

    connect(SeasideCache::manager(), &QContactManager::collectionsAdded,
            this, &SeasideAddressBookModel::collectionsAdded);
    connect(SeasideCache::manager(), &QContactManager::collectionsRemoved,
            this, &SeasideAddressBookModel::collectionsRemoved);
    connect(SeasideCache::manager(), &QContactManager::collectionsChanged,
            this, &SeasideAddressBookModel::collectionsChanged);

    refilter();
}

SeasideAddressBookModel::~SeasideAddressBookModel()
{
}

void SeasideAddressBookModel::setContactId(int contactId)
{
    if (m_contactId != contactId) {
        m_contactId = contactId;
        refilter();
        emit contactIdChanged();
    }
}

int SeasideAddressBookModel::contactId() const
{
    return m_contactId;
}

SeasideAddressBook SeasideAddressBookModel::addressBookAt(int index) const
{
    return m_filteredAddressBooks.value(index);
}

QHash<int, QByteArray> SeasideAddressBookModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(AddressBookRole, "addressBook");
    return roles;
}

int SeasideAddressBookModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid()
            ? m_filteredAddressBooks.count()
            : 0;
}

QVariant SeasideAddressBookModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filteredAddressBooks.count())
        return QVariant();

    switch (role) {
    case AddressBookRole:
        return QVariant::fromValue(m_filteredAddressBooks.at(index.row()));
    }

    return QVariant();
}

void SeasideAddressBookModel::classBegin()
{
}

void SeasideAddressBookModel::componentComplete()
{
    m_complete = true;
    refilter();
}

void SeasideAddressBookModel::collectionsAdded(const QList<QContactCollectionId> &collectionIds)
{
    QList<QContactCollectionId> collectionsMatchingFilter;
    for (const QContactCollectionId &id : collectionIds) {
        if (matchesFilter(id)) {
            collectionsMatchingFilter.append(id);
        }
    }

    if (collectionsMatchingFilter.count() > 0) {
        beginInsertRows(QModelIndex(),
                        m_filteredAddressBooks.count(),
                        m_filteredAddressBooks.count() + collectionsMatchingFilter.count() - 1);
    }
    for (const QContactCollectionId &id : collectionsMatchingFilter) {
        m_filteredAddressBooks.append(SeasideAddressBook::fromCollectionId(id));
    }
    for (const QContactCollectionId &id : collectionIds) {
        m_addressBooks.append(SeasideAddressBook::fromCollectionId(id));
    }
    if (collectionsMatchingFilter.count() > 0) {
        endInsertRows();
        emit countChanged();
    }
}

void SeasideAddressBookModel::collectionsRemoved(const QList<QContactCollectionId> &collectionIds)
{
    for (const QContactCollectionId &id : collectionIds) {
        const int i = findCollection(id);
        if (i >= 0) {
            const int filteredIndex = findFilteredCollection(id);
            if (filteredIndex >= 0) {
                beginRemoveRows(QModelIndex(), filteredIndex, filteredIndex);
                m_filteredAddressBooks.removeAt(filteredIndex);
            }
            m_addressBooks.removeAt(i);
            if (filteredIndex >= 0) {
                endRemoveRows();
                emit countChanged();
            }
        }
    }
}

void SeasideAddressBookModel::collectionsChanged(const QList<QContactCollectionId> &collectionIds)
{
    int startRow = 0;
    int endRow = 0;
    bool emitDataChanged = false;

    for (const QContactCollectionId &id : collectionIds) {
        const int i = findCollection(id);
        if (i >= 0) {
            const int filteredIndex = findFilteredCollection(id);
            if (filteredIndex >= 0) {
                startRow = qMin(filteredIndex, startRow);
                endRow = qMax(filteredIndex, endRow);
                emitDataChanged = true;
            }
            m_addressBooks.replace(i, SeasideAddressBook::fromCollectionId(id));
        }
    }

    if (emitDataChanged) {
        static const QVector<int> roles = QVector<int>() << AddressBookRole;
        emit dataChanged(createIndex(startRow, 0), createIndex(endRow, 0), roles);
    }
}

bool SeasideAddressBookModel::matchesFilter(const QContactCollectionId &id) const
{
    if (m_contactId <= 0) {
        // No contact filter set, so add all available collections to the model.
        return true;
    }

    if (m_allowedCollections.isEmpty()) {
        // A filter has been set but the constituents have not yet been fetched.
        return true;
    }

    return m_allowedCollections.contains(id);
}

void SeasideAddressBookModel::refilter()
{
    if (!m_complete) {
        return;
    }

    if (m_contactId <= 0) {
        // No filter set, so update immediately
        filteredCollectionsChanged();
    } else {
        // Find the constituents of the contact
        if (!m_relationshipsFetch) {
            m_relationshipsFetch = new QContactRelationshipFetchRequest(this);
            m_relationshipsFetch->setManager(SeasideCache::manager());
            m_relationshipsFetch->setRelationshipType(QContactRelationship::Aggregates());
            connect(m_relationshipsFetch, &QContactAbstractRequest::stateChanged,
                    this, &SeasideAddressBookModel::requestStateChanged);
        }
        if (m_relationshipsFetch->state() == QContactAbstractRequest::ActiveState
                && !m_relationshipsFetch->cancel()) {
            qmlInfo(this) << "Unable to filter address books, cannot cancel active relationship request";
            return;
        }
        m_allowedCollections.clear();
        m_relationshipsFetch->setFirst(SeasideCache::apiId(m_contactId));
        m_relationshipsFetch->start();
    }
}

void SeasideAddressBookModel::requestStateChanged(QContactAbstractRequest::State state)
{
    if (state != QContactAbstractRequest::FinishedState)
        return;

    // For each constituent of the contact, add the constituent's collections to the list of
    // (unique) allowed collections.
    for (const QContactRelationship &rel : m_relationshipsFetch->relationships()) {
        if (rel.relationshipType() == QContactRelationship::Aggregates()) {
            const QContactId constituentId = rel.second();
            const QContactCollectionId collectionId =
                    SeasideCache::manager()->contact(constituentId).collectionId();
            if (!m_allowedCollections.contains(collectionId)) {
                m_allowedCollections.append(collectionId);
            }
        }
    }

    filteredCollectionsChanged();
}

void SeasideAddressBookModel::filteredCollectionsChanged()
{
    const int prevCount = rowCount();
    beginResetModel();
    m_filteredAddressBooks.clear();

    for (const SeasideAddressBook &addressBook : m_addressBooks) {
        if (matchesFilter(addressBook.collectionId)) {
            m_filteredAddressBooks.append(addressBook);
        }
    }

    endResetModel();
    if (prevCount != rowCount()) {
        emit countChanged();
    }
}

int SeasideAddressBookModel::findCollection(const QContactCollectionId &id) const
{
    for (int i = 0; i < m_addressBooks.count(); ++i) {
        if (m_addressBooks.at(i).collectionId == id) {
            return i;
        }
    }
    return -1;
}

int SeasideAddressBookModel::findFilteredCollection(const QContactCollectionId &id) const
{
    for (int i = 0; i < m_filteredAddressBooks.count(); ++i) {
        if (m_filteredAddressBooks.at(i).collectionId == id) {
            return i;
        }
    }
    return -1;
}
