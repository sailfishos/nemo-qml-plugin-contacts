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

#ifndef SEASIDEPEOPLENAMEGROUPMODEL_H
#define SEASIDEPEOPLENAMEGROUPMODEL_H

#include <seasidefilteredmodel.h>

#include <QQmlParserStatus>
#include <QAbstractListModel>
#include <QStringList>
#include <QContactId>

QTCONTACTS_USE_NAMESPACE

class SeasideDisplayLabelGroup
{
public:
    SeasideDisplayLabelGroup() {}
    SeasideDisplayLabelGroup(const QString &n, const QSet<quint32> &ids = QSet<quint32>())
        : name(n), contactIds(ids)
    {
        hasContacts = contactIds.count() > 0;
    }

    inline bool operator==(const SeasideDisplayLabelGroup &other) { return other.name == name; }

    QString name;
    bool hasContacts = false;
    QSet<quint32> contactIds;
};

class SeasideDisplayLabelGroupModel : public QAbstractListModel, public QQmlParserStatus, public SeasideDisplayLabelGroupChangeListener
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int minimumCount READ minimumCount CONSTANT)
    Q_PROPERTY(int maximumCount WRITE setMaximumCount READ maximumCount NOTIFY maximumCountChanged)
    Q_PROPERTY(int requiredProperty READ requiredProperty WRITE setRequiredProperty NOTIFY requiredPropertyChanged)

public:
    enum Role {
        NameRole = Qt::UserRole,
        CompressedRole,
        CompressedContentRole
    };
    Q_ENUM(Role)

    enum RequiredPropertyType {
        NoPropertyRequired = SeasideFilteredModel::NoPropertyRequired,
        AccountUriRequired = SeasideFilteredModel::AccountUriRequired,
        PhoneNumberRequired = SeasideFilteredModel::PhoneNumberRequired,
        EmailAddressRequired = SeasideFilteredModel::EmailAddressRequired
    };
    Q_ENUM(RequiredPropertyType)

    SeasideDisplayLabelGroupModel(QObject *parent = 0);
    ~SeasideDisplayLabelGroupModel();

    int requiredProperty() const;
    void setRequiredProperty(int type);

    int minimumCount() const;
    int maximumCount() const;
    void setMaximumCount(int maximumCount);

    Q_INVOKABLE int indexOf(const QString &name) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QVariant get(int row, int role) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void displayLabelGroupsUpdated(const QHash<QString, QSet<quint32> > &groups);

    QHash<int, QByteArray> roleNames() const override;
    void classBegin() override;
    void componentComplete() override;

signals:
    void countChanged();
    void maximumCountChanged();
    void requiredPropertyChanged();

private:
    bool hasFilteredContacts(const QSet<quint32> &contactIds) const;
    void reloadCompressedGroups();
    void reloadGroupIndices();

    QList<SeasideDisplayLabelGroup> m_groups;
    QStringList m_compressedGroups;
    QMap<int, QStringList> m_compressedContent;
    QHash<QString, int> m_groupIndices;
    int m_requiredProperty;
    int m_maximumCount;
    bool m_complete;
};

#endif
