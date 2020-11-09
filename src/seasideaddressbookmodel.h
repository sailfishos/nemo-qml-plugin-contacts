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

#ifndef SEASIDEADDRESSBOOKMODEL_H
#define SEASIDEADDRESSBOOKMODEL_H

#include "seasideaddressbook.h"

#include <QContactRelationshipFetchRequest>

#include <QAbstractListModel>
#include <QList>
#include <QQmlParserStatus>

QTCONTACTS_USE_NAMESPACE

class SeasideAddressBookModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int contactId READ contactId WRITE setContactId NOTIFY contactIdChanged)

public:
    enum Role {
        AddressBookRole = Qt::UserRole,
    };
    Q_ENUM(Role)

    SeasideAddressBookModel(QObject *parent = 0);
    ~SeasideAddressBookModel();

    void setContactId(int contactId);
    int contactId() const;

    Q_INVOKABLE QVariant addressBookAt(int index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged();
    void contactIdChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    void collectionsAdded(const QList<QContactCollectionId> &collectionIds);
    void collectionsRemoved(const QList<QContactCollectionId> &collectionIds);
    void collectionsChanged(const QList<QContactCollectionId> &collectionIds);
    int findCollection(const QContactCollectionId &id) const;
    int findFilteredCollection(const QContactCollectionId &id) const;
    bool matchesFilter(const QContactCollectionId &id) const;
    void refilter();
    void filteredCollectionsChanged();
    void requestStateChanged(QContactAbstractRequest::State state);

    QList<SeasideAddressBook> m_addressBooks;
    QList<SeasideAddressBook> m_filteredAddressBooks;
    QList<QContactCollectionId> m_allowedCollections;
    QContactRelationshipFetchRequest *m_relationshipsFetch = nullptr;
    int m_contactId = -1;
    bool m_complete = false;
 };

#endif
