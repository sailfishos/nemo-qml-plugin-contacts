/*
 * Copyright (c) 2013 - 2020 Jolla Ltd.
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

#ifndef SEASIDECACHE_H
#define SEASIDECACHE_H

#include "contactcacheexport.h"
#include "cacheconfiguration.h"

#include <qtcontacts-extensions.h>
#include <QContactStatusFlags>

#include <QContact>
#include <QContactManager>
#include <QContactFetchRequest>
#include <QContactFetchByIdRequest>
#include <QContactRemoveRequest>
#include <QContactSaveRequest>
#include <QContactRelationshipFetchRequest>
#include <QContactRelationshipSaveRequest>
#include <QContactRelationshipRemoveRequest>
#include <QContactIdFilter>
#include <QContactIdFetchRequest>
#include <QContactName>
#include <QContactCollectionId>

#include <QTranslator>
#include <QBasicTimer>
#include <QHash>
#include <QSet>

#include <QElapsedTimer>
#include <QAbstractListModel>

QTCONTACTS_USE_NAMESPACE

class CONTACTCACHE_EXPORT SeasideDisplayLabelGroupChangeListener
{
public:
    SeasideDisplayLabelGroupChangeListener() {}
    ~SeasideDisplayLabelGroupChangeListener() {}

    virtual void displayLabelGroupsUpdated(const QHash<QString, QSet<quint32> > &groups) = 0;
};

class CONTACTCACHE_EXPORT SeasideCache : public QObject
{
    Q_OBJECT
public:
    enum FilterType {
        FilterNone,
        FilterAll,
        FilterFavorites,
        FilterTypesCount
    };
    Q_ENUM(FilterType)

    enum FetchDataType {
        FetchNone = 0,
        FetchAccountUri = (1 << 0),
        FetchPhoneNumber = (1 << 1),
        FetchEmailAddress = (1 << 2),
        FetchOrganization = (1 << 3),
        FetchAvatar = (1 << 4),
        FetchTypesMask = (FetchAccountUri |
                          FetchPhoneNumber |
                          FetchEmailAddress |
                          FetchOrganization |
                          FetchAvatar)
    };

    enum DisplayLabelOrder {
        FirstNameFirst = CacheConfiguration::FirstNameFirst,
        LastNameFirst = CacheConfiguration::LastNameFirst
    };
    Q_ENUM(DisplayLabelOrder)

    enum ContactState {
        ContactAbsent,
        ContactPartial,
        ContactRequested,
        ContactComplete
    };
    Q_ENUM(ContactState)

    enum {
        // Must be after the highest bit used in QContactStatusFlags::Flag
        HasValidOnlineAccount = (QContactStatusFlags::IsOnline << 1)
    };

    struct ItemData
    {
        virtual ~ItemData() {}

        virtual void displayLabelOrderChanged(DisplayLabelOrder order) = 0;

        virtual void updateContact(const QContact &newContact, QContact *oldContact, ContactState state) = 0;

        virtual void constituentsFetched(const QList<int> &ids) = 0;
        virtual void mergeCandidatesFetched(const QList<int> &ids) = 0;
        virtual void aggregationOperationCompleted() = 0;

        virtual QList<int> constituents() const = 0;
    };

    struct CacheItem;
    struct ItemListener
    {
        ItemListener() : next(0), key(0) {}
        virtual ~ItemListener() {}

        virtual void itemUpdated(CacheItem *item) = 0;
        virtual void itemAboutToBeRemoved(CacheItem *item) = 0;

        ItemListener *next;
        void *key;
    };

    struct CachedPhoneNumber
    {
        CachedPhoneNumber() {}
        CachedPhoneNumber(const QString &n, quint32 i) : normalizedNumber(n), iid(i) {}
        CachedPhoneNumber(const CachedPhoneNumber &other) : normalizedNumber(other.normalizedNumber), iid(other.iid) {}

        bool operator==(const CachedPhoneNumber &other) const
        {
            return other.normalizedNumber == normalizedNumber && other.iid == iid;
        }

        QString normalizedNumber;
        quint32 iid;
    };

    struct CacheItem
    {
        CacheItem() : itemData(0), iid(0), statusFlags(0), contactState(ContactAbsent), listeners(0), filterMatchRole(-1) {}
        CacheItem(const QContact &contact)
            : contact(contact), itemData(0), iid(internalId(contact)),
              statusFlags(contact.detail<QContactStatusFlags>().flagsValue()), contactState(ContactAbsent), listeners(0),
              filterMatchRole(-1) {}

        QContactId apiId() const { return SeasideCache::apiId(contact); }

        ItemListener *appendListener(ItemListener *listener, void *key)
        {
            if (listeners) {
                ItemListener *existing(listeners);
                while (existing->next) {
                    existing = existing->next;
                }
                existing->next = listener;
            } else {
                listeners = listener;
            }

            listener->next = 0;
            listener->key = key;
            return listener;
        }

        bool removeListener(ItemListener *listener)
        {
            if (listeners) {
                ItemListener *existing(listeners);
                ItemListener **previous = &listeners;

                while (existing) {
                    if (existing == listener) {
                        *previous = listener->next;
                        return true;
                    }
                    previous = &existing->next;
                    existing = existing->next;
                }
            }

            return false;
        }

        ItemListener *listener(void *key)
        {
            ItemListener *existing(listeners);
            while (existing && (existing->key != key) && (existing->next)) {
                existing = existing->next;
            }
            return (existing && (existing->key == key)) ? existing : 0;
        }

        QContact contact;
        ItemData *itemData;
        quint32 iid;
        quint64 statusFlags;
        ContactState contactState;
        ItemListener *listeners;
        QString displayLabelGroup;
        QString displayLabel;
        int filterMatchRole;
    };

    struct ContactLinkRequest
    {
        ContactLinkRequest(const QContactId &id) : contactId(id), constituentsFetched(false) {}
        ContactLinkRequest(const ContactLinkRequest &req) : contactId(req.contactId), constituentsFetched(req.constituentsFetched) {}

        QContactId contactId;
        bool constituentsFetched;
    };

    class ListModel : public QAbstractListModel
    {
    public:
        ListModel(QObject *parent = 0) : QAbstractListModel(parent) {}
        virtual ~ListModel() {}

        virtual void sourceAboutToRemoveItems(int begin, int end) = 0;
        virtual void sourceItemsRemoved() = 0;

        virtual void sourceAboutToInsertItems(int begin, int end) = 0;
        virtual void sourceItemsInserted(int begin, int end) = 0;

        virtual void sourceDataChanged(int begin, int end) = 0;

        virtual void sourceItemsChanged() = 0;

        virtual void makePopulated() = 0;
        virtual void updateDisplayLabelOrder() = 0;
        virtual void updateSortProperty() = 0;
        virtual void updateGroupProperty() = 0;

        virtual void updateSectionBucketIndexCache() = 0;

        virtual void saveContactComplete(int localId, int aggregateId) = 0;
    };

    struct ResolveListener
    {
        virtual ~ResolveListener() {}

        virtual void addressResolved(const QString &first, const QString &second, CacheItem *item) = 0;
    };

    struct ChangeListener
    {
        virtual ~ChangeListener() {}

        virtual void itemUpdated(CacheItem *item) = 0;
        virtual void itemAboutToBeRemoved(CacheItem *item) = 0;
    };

    static SeasideCache *instance();
    static QContactManager *manager();

    static QContactId apiId(const QContact &contact);
    static QContactId apiId(quint32 iid);

    static bool validId(const QContactId &id);

    static quint32 internalId(const QContact &contact);
    static quint32 internalId(const QContactId &id);

    static void registerModel(ListModel *model, FilterType type, FetchDataType requiredTypes = FetchNone, FetchDataType extraTypes = FetchNone);
    static void unregisterModel(ListModel *model);

    static void registerUser(QObject *user);
    static void unregisterUser(QObject *user);

    static void registerDisplayLabelGroupChangeListener(SeasideDisplayLabelGroupChangeListener *listener);
    static void unregisterDisplayLabelGroupChangeListener(SeasideDisplayLabelGroupChangeListener *listener);

    static void registerChangeListener(ChangeListener *listener, FetchDataType requiredTypes = FetchNone, FetchDataType extraTypes = FetchNone);
    static void unregisterChangeListener(ChangeListener *listener);

    static void unregisterResolveListener(ResolveListener *listener);

    static DisplayLabelOrder displayLabelOrder();
    static QString sortProperty();
    static QString groupProperty();

    static int contactId(const QContact &contact);
    static int contactId(const QContactId &contactId);

    static CacheItem *existingItem(const QContactId &id);
    static CacheItem *existingItem(quint32 iid);
    static CacheItem *itemById(const QContactId &id, bool requireComplete = true);
    static CacheItem *itemById(int id, bool requireComplete = true);
    static QContactId selfContactId();
    static QContact contactById(const QContactId &id);

    static void ensureCompletion(CacheItem *cacheItem);
    static void refreshContact(CacheItem *cacheItem);

    static QString displayLabelGroup(const CacheItem *cacheItem);
    static QStringList allDisplayLabelGroups();
    static QHash<QString, QSet<quint32> > displayLabelGroupMembers();

    static CacheItem *itemByPhoneNumber(const QString &number, bool requireComplete = true);
    static CacheItem *itemByEmailAddress(const QString &address, bool requireComplete = true);
    static CacheItem *itemByOnlineAccount(const QString &localUid, const QString &remoteUid, bool requireComplete = true);

    static CacheItem *resolvePhoneNumber(ResolveListener *listener, const QString &number, bool requireComplete = true);
    static CacheItem *resolveEmailAddress(ResolveListener *listener, const QString &address, bool requireComplete = true);
    static CacheItem *resolveOnlineAccount(ResolveListener *listener, const QString &localUid, const QString &remoteUid, bool requireComplete = true);

    static bool saveContact(const QContact &contact);
    static bool saveContacts(const QList<QContact> &contacts);
    static bool removeContact(const QContact &contact);
    static bool removeContacts(const QList<QContact> &contacts);

    static void aggregateContacts(const QContact &contact1, const QContact &contact2);
    static void disaggregateContacts(const QContact &contact1, const QContact &contact2);

    static bool fetchConstituents(const QContact &contact);
    static bool fetchMergeCandidates(const QContact &contact);

    static int importContacts(const QString &path);
    static QString exportContacts();

    static const QList<quint32> *contacts(FilterType filterType);
    static bool isPopulated(FilterType filterType);

    static QString primaryName(const QString &firstName, const QString &lastName);
    static QString secondaryName(const QString &firstName, const QString &lastName);

    static QString placeholderDisplayLabel();
    static void decomposeDisplayLabel(const QString &formattedDisplayLabel, QContactName *nameDetail);
    static QString generateDisplayLabel(const QContact &contact, DisplayLabelOrder order = FirstNameFirst, bool fallbackToNonNameDetails = true);
    static QString generateDisplayLabelFromNonNameDetails(const QContact &contact);
    static QUrl filteredAvatarUrl(const QContact &contact, const QStringList &metadataFragments = QStringList());

    static QString normalizePhoneNumber(const QString &input, bool validate = false);
    static QString minimizePhoneNumber(const QString &input, bool validate = false);

    static QContactCollection collectionFromId(const QContactCollectionId &collectionId);
    static QContactCollectionId aggregateCollectionId();
    static QContactCollectionId localCollectionId();

    bool event(QEvent *event);

    // For synchronizeLists()
    int insertRange(int index, int count, const QList<quint32> &source, int sourceIndex) { return insertRange(m_syncFilter, index, count, source, sourceIndex); }
    int removeRange(int index, int count) { removeRange(m_syncFilter, index, count); return 0; }

protected:
    void timerEvent(QTimerEvent *event);
    void setSortOrder(const QString &property);
    void startRequest(bool *idleProcessing);

private slots:
    void contactsAvailable();
    void contactIdsAvailable();
    void relationshipsAvailable();
    void requestStateChanged(QContactAbstractRequest::State state);
    void addressRequestStateChanged(QContactAbstractRequest::State state);
    void dataChanged();
    void contactsAdded(const QList<QContactId> &contactIds);
    void contactsChanged(const QList<QContactId> &contactIds, const QList<QContactDetail::DetailType> &typesChanged);
    void contactsPresenceChanged(const QList<QContactId> &contactIds);
    void contactsRemoved(const QList<QContactId> &contactIds);
    void displayLabelGroupsChanged(const QStringList &groups);
    void displayLabelOrderChanged(CacheConfiguration::DisplayLabelOrder order);
    void sortPropertyChanged(const QString &sortProperty);
    void displayStatusChanged(const QString &);

private:
    enum PopulateProgress {
        Unpopulated,
        FetchFavorites,
        FetchMetadata,
        Populated
    };

    SeasideCache();
    ~SeasideCache();

    static void checkForExpiry();

    void keepPopulated(quint32 requiredTypes, quint32 extraTypes);

    void requestUpdate();
    void appendContacts(const QList<QContact> &contacts, FilterType filterType, bool partialFetch, const QSet<QContactDetail::DetailType> &queryDetailTypes);
    void fetchContacts();
    void updateContacts(const QList<QContactId> &contactIds, QList<QContactId> *updateList);
    void applyPendingContactUpdates();
    void applyContactUpdates(const QList<QContact> &contacts, const QSet<QContactDetail::DetailType> &queryDetailTypes);
    void updateSectionBucketIndexCaches();

    void resolveUnknownAddresses(const QString &first, const QString &second, CacheItem *item);
    bool updateContactIndexing(const QContact &oldContact, const QContact &contact, quint32 iid, const QSet<QContactDetail::DetailType> &queryDetailTypes, CacheItem *item);
    void updateCache(CacheItem *item, const QContact &contact, bool partialFetch, bool initialInsert);
    void reportItemUpdated(CacheItem *item);

    void removeRange(FilterType filter, int index, int count);
    int insertRange(FilterType filter, int index, int count, const QList<quint32> &queryIds, int queryIndex);

    void contactDataChanged(quint32 iid);
    void contactDataChanged(quint32 iid, FilterType filter);
    void removeContactData(quint32 iid, FilterType filter);
    void makePopulated(FilterType filter);

    void addToContactDisplayLabelGroup(quint32 iid, const QString &group, QSet<QString> *modifiedGroups = 0);
    void removeFromContactDisplayLabelGroup(quint32 iid, const QString &group, QSet<QString> *modifiedGroups = 0);
    void notifyDisplayLabelGroupsChanged(const QSet<QString> &groups);

    void updateConstituentAggregations(const QContactId &contactId);
    void completeContactAggregation(const QContactId &contact1Id, const QContactId &contact2Id);

    void resolveAddress(ResolveListener *listener, const QString &first, const QString &second, bool requireComplete);

    CacheItem *itemMatchingPhoneNumber(const QString &number, const QString &normalized, bool requireComplete);

    int contactIndex(quint32 iid, FilterType filter);

    QContactFilter filterForMergeCandidates(const QContact &contact) const;
    QContactFilter aggregateFilter() const;
    bool ignoreContactForDisplayLabelGroups(const QContact &contact) const;

    void notifySaveContactComplete(int constituentId, int aggregateId);

    static QContactRelationship makeRelationship(const QString &type, const QContactId &id1, const QContactId &id2);

    QList<quint32> m_contacts[FilterTypesCount];

    QBasicTimer m_expiryTimer;
    QBasicTimer m_fetchTimer;
    QHash<quint32, CacheItem> m_people;
    QMultiHash<QString, CachedPhoneNumber> m_phoneNumberIds;
    QHash<QString, quint32> m_emailAddressIds;
    QHash<QPair<QString, QString>, quint32> m_onlineAccountIds;
    QHash<QContactId, QContact> m_contactsToSave;
    QHash<QString, QSet<quint32> > m_contactDisplayLabelGroups;
    QList<QContact> m_contactsToCreate;
    QHash<FilterType, QPair<QSet<QContactDetail::DetailType>, QList<QContact> > > m_contactsToAppend;
    QList<QPair<QSet<QContactDetail::DetailType>, QList<QContact> > > m_contactsToUpdate;
    QList<QContactId> m_contactsToRemove;
    QList<QContactId> m_changedContacts;
    QList<QContactId> m_presenceChangedContacts;
    QSet<QContactId> m_aggregatedContacts;
    QList<QContactId> m_contactsToFetchConstituents;
    QList<QContactId> m_contactsToFetchCandidates;
    QList<QContactId> m_contactsToLinkTo;
    QList<QPair<ContactLinkRequest, ContactLinkRequest> > m_contactPairsToLink;
    QList<QContactRelationship> m_relationshipsToSave;
    QList<QContactRelationship> m_relationshipsToRemove;
    QList<SeasideDisplayLabelGroupChangeListener*> m_displayLabelGroupChangeListeners;
    QList<ChangeListener*> m_changeListeners;
    QList<ListModel *> m_models[FilterTypesCount];
    QSet<QObject *> m_users;
    QHash<QContactId,int> m_expiredContacts;
    QContactFetchRequest m_fetchRequest;
    QContactFetchByIdRequest m_fetchByIdRequest;
    QContactIdFetchRequest m_contactIdRequest;
    QContactRelationshipFetchRequest m_relationshipsFetchRequest;
    QContactRemoveRequest m_removeRequest;
    QContactSaveRequest m_saveRequest;
    QContactRelationshipSaveRequest m_relationshipSaveRequest;
    QContactRelationshipRemoveRequest m_relationshipRemoveRequest;
    QList<QContactSortOrder> m_sortOrder;
    QList<QContactSortOrder> m_onlineSortOrder;
    FilterType m_syncFilter;
    int m_populated;
    int m_cacheIndex;
    int m_queryIndex;
    int m_fetchProcessedCount;
    int m_fetchByIdProcessedCount;
    DisplayLabelOrder m_displayLabelOrder;
    QString m_sortProperty;
    QString m_groupProperty;
    bool m_keepPopulated;
    PopulateProgress m_populateProgress;
    bool m_populating; // true if current m_fetchRequest makes progress
    quint32 m_fetchTypes;
    quint32 m_extraFetchTypes;
    quint32 m_dataTypesFetched;
    bool m_updatesPending;
    bool m_refreshRequired;
    bool m_contactsUpdated;
    bool m_displayOff;
    QSet<QContactId> m_constituentIds;
    QSet<QContactId> m_candidateIds;

    struct ResolveData {
        QString first;
        QString second;
        QString compare; // only used in m_unknownAddresses
        bool requireComplete;
        ResolveListener *listener;
    };
    QHash<QContactFetchRequest *, ResolveData> m_resolveAddresses;
    QSet<ResolveData> m_pendingResolve; // these have active requests already
    QList<ResolveData> m_unknownResolveAddresses;
    QList<ResolveData> m_unknownAddresses;
    QSet<QString> m_resolvedPhoneNumbers;

    QElapsedTimer m_timer;
    QElapsedTimer m_fetchPostponed;

    static SeasideCache *instancePtr;
    static int contactDisplayLabelGroupCount;
    static QStringList allContactDisplayLabelGroups;
    static QTranslator *engEnTranslator;
    static QTranslator *translator;

    friend bool operator==(const SeasideCache::ResolveData &lhs, const SeasideCache::ResolveData &rhs);
    friend uint qHash(const SeasideCache::ResolveData &key, uint seed);
};

bool operator==(const SeasideCache::ResolveData &lhs, const SeasideCache::ResolveData &rhs);
uint qHash(const SeasideCache::ResolveData &key, uint seed = 0);

#endif
