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

#ifndef SEASIDESIMPLECONTACTMODEL_H
#define SEASIDESIMPLECONTACTMODEL_H

#include <seasidecache.h>
#include <seasideaddressbook.h>

#include <QQmlParserStatus>
#include <QAbstractListModel>

class SeasidePerson;

QTCONTACTS_USE_NAMESPACE

class SeasideSimpleContactModel : public QAbstractListModel,
                                  public QQmlParserStatus,
                                  public SeasideCache::ChangeListener
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool populated READ isPopulated NOTIFY populatedChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole,
        PrimaryNameRole,
        SecondaryNameRole,
        DisplayLabelRole,
        AddressBookRole,
    };
    Q_ENUM(Role)

    SeasideSimpleContactModel(QObject *parent = 0);
    ~SeasideSimpleContactModel();

    bool isPopulated() const;

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    virtual void classBegin() override;
    virtual void componentComplete() override;

Q_SIGNALS:
    void populatedChanged();
    void countChanged();

protected:
    class ContactInfo
    {
    public:
        ContactInfo() {}
        ContactInfo(SeasideCache::CacheItem *cacheItem);
        ~ContactInfo() {}

        SeasideAddressBook addressBook;
        SeasideCache::CacheItem *cacheItem = nullptr;
    };

    virtual void reset() = 0;
    virtual void itemUpdated(SeasideCache::CacheItem *item);
    virtual void itemAboutToBeRemoved(SeasideCache::CacheItem *item);
    void setContactIds(const QList<int> &contactIds);
    void updateOrReset(const QList<int> &newContactIds);

    static QString getPrimaryName(SeasideCache::CacheItem *item);

    QList<ContactInfo> m_contacts;
    bool m_complete = false;
    bool m_populated = false;
};

#endif
