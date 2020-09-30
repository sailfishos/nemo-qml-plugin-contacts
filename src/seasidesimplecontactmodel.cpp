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

#include "seasidesimplecontactmodel.h"
#include "seasideperson.h"
#include "synchronizelists.h"

#include <QQmlInfo>

#include <QSet>
#include <QDebug>

namespace {

const QByteArray idRole("id");
const QByteArray addressBookRole("addressBook");
const QByteArray displayLabelRole("displayLabel");

}

SeasideSimpleContactModel::ContactInfo::ContactInfo(SeasideCache::CacheItem *cacheItem)
    : cacheItem(cacheItem)
{
    if (cacheItem) {
        addressBook = SeasideAddressBook::fromCollectionId(cacheItem->contact.collectionId());
    } else {
        qWarning() << "Invalid ContactInfo cache item!";
    }
}


SeasideSimpleContactModel::SeasideSimpleContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
    SeasideCache::registerChangeListener(this);
}

SeasideSimpleContactModel::~SeasideSimpleContactModel()
{
    SeasideCache::unregisterChangeListener(this);
}

bool SeasideSimpleContactModel::isPopulated() const
{
    return m_populated;
}

QHash<int, QByteArray> SeasideSimpleContactModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(IdRole, idRole);
    roles.insert(DisplayLabelRole, displayLabelRole);
    roles.insert(AddressBookRole, addressBookRole);
    return roles;
}

int SeasideSimpleContactModel::rowCount(const QModelIndex &) const
{
    return m_contacts.count();
}

QVariant SeasideSimpleContactModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_contacts.count()) {
        return QVariant();
    }

    const ContactInfo &contactInfo = m_contacts.at(index.row());

    switch (role) {
    case IdRole:
        return contactInfo.cacheItem->iid;
    case DisplayLabelRole:
        return contactInfo.cacheItem->displayLabel;
    case AddressBookRole:
        return QVariant::fromValue(contactInfo.addressBook);
    }

    return QVariant();
}

void SeasideSimpleContactModel::classBegin()
{
}

void SeasideSimpleContactModel::componentComplete()
{
    m_complete = true;
    reset();
}

void SeasideSimpleContactModel::updateOrReset(const QList<int> &newContactIds)
{
    QList<int> currentContactIds;
    for (const ContactInfo &contactInfo : m_contacts) {
        currentContactIds.append(contactInfo.cacheItem->iid);
    }

    const QSet<int> currentIdSet = currentContactIds.toSet();
    const QSet<int> newIdSet = newContactIds.toSet();

    if (currentIdSet == newIdSet) {
        return;
    }

    const int delta = newIdSet.count() - currentIdSet.count();

    if (delta == -1) {
        // Remove one item
        for (int i = 0; i < m_contacts.count(); ++i) {
            quint32 contactId = m_contacts.at(i).cacheItem->iid;
            if (!newIdSet.contains(contactId)) {
                beginRemoveRows(QModelIndex(), i, i);
                m_contacts.removeAt(i);
                endRemoveRows();
                return;
            }
        }
    }

    if (delta == 1) {
        // Insert one item
        for (int i = 0; i < newContactIds.count(); ++i) {
            if (newContactIds.at(i) != currentContactIds.value(i)) {
                if (newContactIds.mid(i + 1) == currentContactIds.mid(i)) {
                    // This is a new contact, insert it
                    SeasideCache::CacheItem *cacheItem = SeasideCache::itemById(newContactIds.at(i));
                    if (cacheItem) {
                        beginInsertRows(QModelIndex(), i, i);
                        m_contacts.insert(i, ContactInfo(cacheItem));
                        endInsertRows();
                        return;
                    }
                } else {
                    // There are other list ordering changes besides the addition of the new
                    // contact, so reset the whole model.
                    break;
                }
            }
        }
    }

    beginResetModel();
    m_contacts.clear();

    for (int contactId : newContactIds) {
        SeasideCache::CacheItem *cacheItem = SeasideCache::itemById(contactId);
        if (!cacheItem) {
            qmlInfo(this) << "No cache item found for id:" << contactId;
            continue;
        }
        m_contacts.append(ContactInfo(cacheItem));
    }

    endResetModel();
}

void SeasideSimpleContactModel::setContactIds(const QList<int> &contactIds)
{
    const int prevCount = m_contacts.count();

    updateOrReset(contactIds);

    if (!m_populated) {
        m_populated = true;
        emit populatedChanged();
    }

    if (prevCount != m_contacts.count()) {
        emit countChanged();
    }
}

void SeasideSimpleContactModel::itemUpdated(SeasideCache::CacheItem *item)
{
    for (int i = 0; i < m_contacts.count(); ++i) {
        if (m_contacts[i].cacheItem == item) {
            QVector<int> roles;
            if (m_contacts[i].cacheItem->iid != item->iid) {
                roles << IdRole;
            }
            if (m_contacts[i].cacheItem->displayLabel != item->displayLabel) {
                roles << DisplayLabelRole;
            }
            if (m_contacts[i].addressBook != SeasideAddressBook::fromCollectionId(item->contact.collectionId())) {
                roles << AddressBookRole;
            }
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), roles);
            break;
        }
    }
}

void SeasideSimpleContactModel::itemAboutToBeRemoved(SeasideCache::CacheItem *item)
{
    for (int i = 0; i < m_contacts.count(); ++i) {
        if (m_contacts[i].cacheItem == item) {
            beginRemoveRows(QModelIndex(), i, i);
            m_contacts.removeAt(i);
            endRemoveRows();
            break;
        }
    }
}
