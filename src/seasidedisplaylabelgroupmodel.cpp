/*
 * Copyright (c) 2013 - 2020 Jolla Ltd.
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

#include "seasidedisplaylabelgroupmodel.h"
#include "seasidestringlistcompressor.h"

#include <seasidecache.h>

#include <QQmlInfo>
#include <QContactStatusFlags>
#include <QContactOnlineAccount>
#include <QContactPhoneNumber>
#include <QContactEmailAddress>
#include <QDebug>

/*!
  \qmltype PeopleDisplayLabelGroupModel
  \inqmlmodule org.nemomobile.contacts
*/
SeasideDisplayLabelGroupModel::SeasideDisplayLabelGroupModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_requiredProperty(NoPropertyRequired)
    , m_maximumCount(std::numeric_limits<int>::max())
    , m_complete(false)
{
    SeasideCache::registerDisplayLabelGroupChangeListener(this);

    const QStringList &allGroups = SeasideCache::allDisplayLabelGroups();
    QHash<QString, QSet<quint32> > existingGroups = SeasideCache::displayLabelGroupMembers();
    if (!existingGroups.isEmpty()) {
        for (int i=0; i<allGroups.count(); i++)
            m_groups << SeasideDisplayLabelGroup(allGroups[i], existingGroups.value(allGroups[i]));
    } else {
        for (int i=0; i<allGroups.count(); i++)
            m_groups << SeasideDisplayLabelGroup(allGroups[i]);
    }
}

SeasideDisplayLabelGroupModel::~SeasideDisplayLabelGroupModel()
{
    SeasideCache::unregisterDisplayLabelGroupChangeListener(this);
}

/*!
  \qmlproperty int PeopleDisplayLabelGroupModel::requiredProperty
*/
int SeasideDisplayLabelGroupModel::requiredProperty() const
{
    return m_requiredProperty;
}

void SeasideDisplayLabelGroupModel::setRequiredProperty(int properties)
{
    if (m_requiredProperty != properties) {
        m_requiredProperty = properties;

        // Update hasContacts
        bool needsRecompression = false;
        QList<SeasideDisplayLabelGroup>::iterator it = m_groups.begin(), end = m_groups.end();
        for ( ; it != end; ++it) {
            SeasideDisplayLabelGroup &existing(*it);
            bool hasContacts = hasFilteredContacts(existing.contactIds);
            if (existing.hasContacts != hasContacts) {
                existing.hasContacts = hasContacts;
                needsRecompression = true;
            }
        }

        emit requiredPropertyChanged();

        if (needsRecompression) {
            reloadCompressedGroups();
        }
    }
}

/*!
  \qmlproperty int PeopleDisplayLabelGroupModel::minimumCount
*/
int SeasideDisplayLabelGroupModel::minimumCount() const
{
    return SeasideStringListCompressor::minimumCompressionInputCount();
}

/*!
  \qmlproperty int PeopleDisplayLabelGroupModel::maximumCount
*/
int SeasideDisplayLabelGroupModel::maximumCount() const
{
    return m_maximumCount;
}

void SeasideDisplayLabelGroupModel::setMaximumCount(int maximumCount)
{
    maximumCount = qMax(minimumCount(), maximumCount);
    if (maximumCount != m_maximumCount) {
        m_maximumCount = maximumCount;
        emit maximumCountChanged();

        reloadCompressedGroups();
    }
}

/*!
  \qmlmethod int PeopleDisplayLabelGroupModel::indexOf(string name)
*/
int SeasideDisplayLabelGroupModel::indexOf(const QString &name) const
{
    return m_groupIndices.value(name);
}

/*!
  \qmlmethod object PeopleDisplayLabelGroupModel::get(int row)
*/
QVariantMap SeasideDisplayLabelGroupModel::get(int row) const
{
    if (row < 0 || row > m_compressedGroups.count()) {
        return QVariantMap();
    }

    QVariantMap m;
    QString group = m_compressedGroups.at(row);
    m.insert("name", group);
    m.insert("compressed", SeasideStringListCompressor::isCompressionMarker(group));
    m.insert("compressedContent", m_compressedContent.value(row));
    return m;
}

/*!
  \qmlmethod object PeopleDisplayLabelGroupModel::get(int row, int role)
*/
QVariant SeasideDisplayLabelGroupModel::get(int row, int role) const
{
    if (row < 0 || row >= m_compressedGroups.count()) {
        return QVariant();
    }

    const QString &group = m_compressedGroups.at(row);

    switch (role) {
    case NameRole:
        return group;
    case CompressedRole:
        return SeasideStringListCompressor::isCompressionMarker(group);
    case CompressedContentRole:
        return m_compressedContent.value(row);
    }

    return QVariant();
}

QHash<int, QByteArray> SeasideDisplayLabelGroupModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(NameRole, "name");
    roles.insert(CompressedRole, "compressed");
    roles.insert(CompressedContentRole, "compressedContent");
    return roles;
}

/*!
  \qmlproperty int PeopleDisplayLabelGroupModel::count
*/
int SeasideDisplayLabelGroupModel::rowCount(const QModelIndex &) const
{
    return m_compressedGroups.count();
}

void SeasideDisplayLabelGroupModel::classBegin()
{
}

void SeasideDisplayLabelGroupModel::componentComplete()
{
    m_complete = true;
    reloadCompressedGroups();
}

QVariant SeasideDisplayLabelGroupModel::data(const QModelIndex &index, int role) const
{
    return get(index.row(), role);
}

void SeasideDisplayLabelGroupModel::displayLabelGroupsUpdated(const QHash<QString, QSet<quint32> > &groups)
{
    if (groups.isEmpty())
        return;

    bool wasEmpty = m_groups.isEmpty();
    bool needsRecompression = false;

    if (wasEmpty) {
        const QStringList &allGroups = SeasideCache::allDisplayLabelGroups();
        if (!allGroups.isEmpty()) {
            for (int i=0; i<allGroups.count(); i++) {
                m_groups << SeasideDisplayLabelGroup(allGroups[i]);
            }
            needsRecompression = true;
        }
    }

    QHash<QString, QSet<quint32> >::const_iterator it = groups.constBegin(), end = groups.constEnd();
    for ( ; it != end; ++it) {
        const QString group(it.key());

        QList<SeasideDisplayLabelGroup>::iterator existingIt = m_groups.begin(), existingEnd = m_groups.end();
        for ( ; existingIt != existingEnd; ++existingIt) {
            SeasideDisplayLabelGroup &existing(*existingIt);
            if (existing.name == group) {
                existing.contactIds = it.value();
                bool hasContacts = hasFilteredContacts(existing.contactIds);
                if (existing.hasContacts != hasContacts) {
                    existing.hasContacts = hasContacts;
                    needsRecompression = true;
                }
                break;
            }
        }
        if (existingIt == existingEnd) {
            // Find the index of this group in the groups list
            const QStringList &allGroups = SeasideCache::allDisplayLabelGroups();

            int allIndex = 0;
            int groupIndex = 0;
            for ( ; allIndex < allGroups.size() && allGroups.at(allIndex) != group; ++allIndex) {
                if (m_groups.at(groupIndex).name == allGroups.at(allIndex)) {
                    ++groupIndex;
                }
            }
            if (allIndex < allGroups.count()) {
                // Insert this group
                m_groups.insert(groupIndex, SeasideDisplayLabelGroup(group, it.value()));
                needsRecompression = true;
            } else {
                qWarning() << "Could not find unknown group in allGroups!" << group;
            }
        }
    }

    if (needsRecompression) {
        reloadCompressedGroups();
    }
}

bool SeasideDisplayLabelGroupModel::hasFilteredContacts(const QSet<quint32> &contactIds) const
{
    if (m_requiredProperty != NoPropertyRequired) {
        // Check if these contacts are included by the current filter
        foreach (quint32 iid, contactIds) {
            if (SeasideCache::CacheItem *item = SeasideCache::existingItem(iid)) {
                bool haveMatch = (m_requiredProperty & AccountUriRequired) && (item->statusFlags & QContactStatusFlags::HasOnlineAccount);
                haveMatch |= (m_requiredProperty & PhoneNumberRequired) && (item->statusFlags & QContactStatusFlags::HasPhoneNumber);
                haveMatch |= (m_requiredProperty & EmailAddressRequired) && (item->statusFlags & QContactStatusFlags::HasEmailAddress);
                if (haveMatch) {
                    return true;
                }
            } else {
                qWarning() << "SeasideDisplayLabelGroupModel: obsolete contact" << iid;
            }
        }

        return false;
    }

    return contactIds.count() > 0;
}

void SeasideDisplayLabelGroupModel::reloadGroupIndices()
{
    m_groupIndices.clear();

    // Cache the index of each display label group for a fast lookup in indexOf(). If a group is
    // compressed, all of its contents are inserted into the hash with the same index.
    for (int i = 0; i < m_compressedGroups.count(); i++) {
        const QString &group = m_compressedGroups.at(i);
        if (SeasideStringListCompressor::isCompressionMarker(group)) {
            // An empty default must be passed in, otherwise if the key doesn't exist
            // the reference to the returned default-constructed value can become invalid
            const QStringList &groupContent = m_compressedContent.value(i, QStringList());
            for (const QString &groupContentEntry : groupContent) {
                m_groupIndices.insert(groupContentEntry, i);
            }
        } else {
            m_groupIndices.insert(group, i);
        }
    }
}

void SeasideDisplayLabelGroupModel::reloadCompressedGroups()
{
    if (!m_complete) {
        return;
    }

    QStringList labelGroups;
    for (const SeasideDisplayLabelGroup &group : m_groups) {
        if (group.hasContacts) {
            labelGroups.append(group.name);
        }
    }

    QMap<int, QStringList> compressedContent;
    QStringList compressedGroups = SeasideStringListCompressor::compress(labelGroups, m_maximumCount, &compressedContent);
    if (compressedGroups.count() < minimumCount()) {
        compressedGroups.clear();
    }

    if (m_compressedGroups != compressedGroups) {
        const bool countChanging = m_compressedGroups.count() != compressedGroups.count();
        beginResetModel();
        m_compressedGroups = compressedGroups;
        m_compressedContent = compressedContent;
        reloadGroupIndices();
        endResetModel();
        if (countChanging) {
            emit countChanged();
        }
    }
}
