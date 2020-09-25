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
}

SeasideAddressBookModel::~SeasideAddressBookModel()
{
}

SeasideAddressBook SeasideAddressBookModel::addressBookAt(int index) const
{
    return m_addressBooks.value(index);
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
            ? m_addressBooks.count()
            : 0;
}

QVariant SeasideAddressBookModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_addressBooks.count())
        return QVariant();

    switch (role) {
    case AddressBookRole:
        return QVariant::fromValue(m_addressBooks.at(index.row()));
    }

    return QVariant();
}

void SeasideAddressBookModel::collectionsAdded(const QList<QContactCollectionId> &collectionIds)
{
    beginInsertRows(QModelIndex(), m_addressBooks.count(), m_addressBooks.count() + collectionIds.count() - 1);
    for (const QContactCollectionId &id : collectionIds) {
        m_addressBooks.append(SeasideAddressBook::fromCollectionId(id));
    }
    endInsertRows();
    emit countChanged();
}

void SeasideAddressBookModel::collectionsRemoved(const QList<QContactCollectionId> &collectionIds)
{
    for (const QContactCollectionId &id : collectionIds) {
        int i = findCollection(id);
        if (i >= 0) {
            beginRemoveRows(QModelIndex(), i, i);
            m_addressBooks.removeAt (i);
            endRemoveRows();
            emit countChanged();
        }
    }
}

void SeasideAddressBookModel::collectionsChanged(const QList<QContactCollectionId> &collectionIds)
{
    int startRow = 0;
    int endRow = 0;
    for (const QContactCollectionId &id : collectionIds) {
        int i = findCollection(id);
        if (i >= 0) {
            startRow = qMin(i, startRow);
            endRow = qMax(i, endRow);
            m_addressBooks.replace(i, SeasideAddressBook::fromCollectionId(id));
        }
    }

    static const QVector<int> roles = QVector<int>() << AddressBookRole;
    emit dataChanged(createIndex(startRow, 0), createIndex(endRow, 0), roles);
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
