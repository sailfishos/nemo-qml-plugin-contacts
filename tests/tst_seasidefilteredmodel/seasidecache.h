#ifndef SEASIDECACHE_H
#define SEASIDECACHE_H

#include <qtcontacts-extensions.h>
#include <QContactStatusFlags>

#include <QContact>
#include <QContactId>
#include <QContactManager>
#include <QContactCollection>
#include <QContactAvatar>
#include <QContactName>

#include <QAbstractListModel>

// Provide enough of SeasideCache's interface to support SeasideFilteredModel

QTCONTACTS_USE_NAMESPACE

class SeasidePerson;
class SeasideActionPlugin;

class SeasideCache : public QObject
{
    Q_OBJECT
public:
    enum FilterType {
        FilterNone,
        FilterAll,
        FilterFavorites,
        FilterTypesCount
    };

    enum FetchDataType {
        FetchNone = 0,
        FetchAccountUri = (1 << 0),
        FetchPhoneNumber = (1 << 1),
        FetchEmailAddress = (1 << 2),
        FetchOrganization = (1 << 3),
        FetchTypesMask = (FetchAccountUri |
                          FetchPhoneNumber |
                          FetchEmailAddress |
                          FetchOrganization)
    };

    enum DisplayLabelOrder {
        FirstNameFirst = 0,
        LastNameFirst
    };

    enum ContactState {
        ContactAbsent,
        ContactPartial,
        ContactRequested,
        ContactComplete
    };

    enum {
        HasValidOnlineAccount = (QContactStatusFlags::IsOnline << 1)
    };

    struct ItemData
    {
        virtual ~ItemData() {}

        virtual void displayLabelOrderChanged(DisplayLabelOrder order) = 0;

        virtual void updateContact(const QContact &newContact, QContact *oldContact, ContactState state) = 0;

        virtual void constituentsFetched(const QList<int> &ids) = 0;
        virtual void mergeCandidatesFetched(const QList<int> &ids) = 0;
    };

    struct CacheItem;
    struct ItemListener
    {
        virtual ~ItemListener() {}

        virtual void itemUpdated(CacheItem *) {};
        virtual void itemAboutToBeRemoved(CacheItem *) {};

        ItemListener *next;
    };

    struct CacheItem
    {
        CacheItem() : itemData(0), iid(0), statusFlags(0), contactState(ContactAbsent), listeners(0), filterMatchRole(-1) {}
        CacheItem(const QContact &contact)
            : contact(contact), itemData(0), iid(internalId(contact)),
              statusFlags(contact.detail<QContactStatusFlags>().flagsValue()), contactState(ContactComplete), listeners(0),
              filterMatchRole(-1) {}

        ItemListener *listener(void *) { return 0; }

        ItemListener *appendListener(ItemListener *listener, void *) { listeners = listener; listener->next = 0; return listener; }
        bool removeListener(ItemListener *) { listeners = 0; return true; }

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
    };

    struct ResolveListener
    {
        virtual ~ResolveListener() {}

        virtual void addressResolved(const QString &, const QString &, CacheItem *item) = 0;
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

    SeasideCache();
    ~SeasideCache();

    static void registerModel(ListModel *model, FilterType type, FetchDataType requiredTypes = FetchNone, FetchDataType extraTypes = FetchNone);
    static void unregisterModel(ListModel *model);

    static void registerUser(QObject *user);
    static void unregisterUser(QObject *user);

    static void registerChangeListener(ChangeListener *listener);
    static void unregisterChangeListener(ChangeListener *listener);

    static void unregisterResolveListener(ResolveListener *listener);

    static DisplayLabelOrder displayLabelOrder();
    static QString sortProperty();
    static QString groupProperty();

    static int contactId(const QContact &contact);

    static CacheItem *existingItem(const QContactId &id);
    static CacheItem *existingItem(quint32 iid);
    static CacheItem *itemById(const QContactId &id, bool requireComplete = true);
    static CacheItem *itemById(int id, bool requireComplete = true);
    static QContactId selfContactId();
    static QContact contactById(const QContactId &id);
    static QString displayLabelGroup(const CacheItem *cacheItem);
    static QStringList allDisplayLabelGroups();

    static void ensureCompletion(CacheItem *cacheItem);
    static void refreshContact(CacheItem *cacheItem);

    static CacheItem *itemByPhoneNumber(const QString &number, bool requireComplete = true);
    static CacheItem *itemByEmailAddress(const QString &email, bool requireComplete = true);
    static CacheItem *itemByOnlineAccount(const QString &localUid, const QString &remoteUid, bool requireComplete = true);

    static CacheItem *resolvePhoneNumber(ResolveListener *listener, const QString &msisdn, bool requireComplete = true);
    static CacheItem *resolveEmailAddress(ResolveListener *listener, const QString &email, bool requireComplete = true);
    static CacheItem *resolveOnlineAccount(ResolveListener *listener, const QString &localUid, const QString &remoteUid, bool requireComplete = true);

    static bool saveContact(const QContact &contact);
    static bool saveContacts(const QList<QContact> &contacts);
    static void removeContact(const QContact &contact);
    static void removeContacts(const QList<QContact> &contact);

    static void aggregateContacts(const QContact &contact1, const QContact &contact2);
    static void disaggregateContacts(const QContact &contact1, const QContact &contact2);

    static void fetchConstituents(const QContact &contact);

    static void fetchMergeCandidates(const QContact &contact);

    static const QList<quint32> *contacts(FilterType filterType);
    static bool isPopulated(FilterType filterType);

    static QString getPrimaryName(const QContact &contact);
    static QString getSecondaryName(const QContact &contact);

    static QString primaryName(const QString &firstName, const QString &lastName);
    static QString secondaryName(const QString &firstName, const QString &lastName);

    static QString placeholderDisplayLabel();
    static void decomposeDisplayLabel(const QString &formattedDisplayLabel, QContactName *nameDetail);
    static QString generateDisplayLabel(const QContact &contact, DisplayLabelOrder order = FirstNameFirst);
    static QString generateDisplayLabelFromNonNameDetails(const QContact &contact);
    static QUrl filteredAvatarUrl(const QContact &contact, const QStringList &metadataFragments = QStringList());
    static bool removeLocalAvatarFile(const QContact &, const QContactAvatar &);

    static QString normalizePhoneNumber(const QString &input, bool validate = false);
    static QString minimizePhoneNumber(const QString &input, bool validate = false);

    static QContactCollectionId aggregateCollectionId();
    static QContactCollectionId localCollectionId();

    static SeasideActionPlugin *actionPlugin();

    void populate(FilterType filterType);
    void insert(FilterType filterType, int index, const QList<quint32> &ids);
    void remove(FilterType filterType, int index, int count);

    static int importContacts(const QString &path);
    static QString exportContacts();

    void setFirstName(FilterType filterType, int index, const QString &name);

    void reset();

    static QList<quint32> getContactsForFilterType(FilterType filterType);

    QList<quint32> m_contacts[FilterTypesCount];
    ListModel *m_models[FilterTypesCount];
    bool m_populated[FilterTypesCount];

    QList<CacheItem> m_cache;
    QHash<quint32, int> m_cacheIndices;

    static SeasideCache *instancePtr;
    static QStringList allContactDisplayLabelGroups;

    quint32 idAt(int index) const;
};


#endif
