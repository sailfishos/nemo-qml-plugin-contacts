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

#include <seasidecache.h>

// Qt
#include <QCoreApplication>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include <QFileInfo>
#include <QDebug>

// Contacts
#include <QContactManager>
#include <QContactDetailFilter>
#include <QContactCollectionFilter>
#include <QContactCollection>

#include <QContactAddress>
#include <QContactAnniversary>
#include <QContactAvatar>
#include <QContactBirthday>
#include <QContactEmailAddress>
#include <QContactFamily>
#include <QContactFavorite>
#include <QContactGender>
#include <QContactGeoLocation>
#include <QContactGuid>
#include <QContactHobby>
#include <QContactName>
#include <QContactNickname>
#include <QContactNote>
#include <QContactOnlineAccount>
#include <QContactOrganization>
#include <QContactPhoneNumber>
#include <QContactPresence>
#include <QContactRingtone>
#include <QContactSyncTarget>
#include <QContactTag>
#include <QContactTimestamp>
#include <QContactUrl>

QTCONTACTS_USE_NAMESPACE

namespace {

QString delimiter(QString::fromLatin1("\n    "));

void errorMessage(const QString &s)
{
    QTextStream ts(stderr);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ts << s << Qt::endl;
#else
    ts << s << endl;
#endif
}

void invalidUsage(const QString &app)
{
    errorMessage(QString::fromLatin1(
        "Usage: %1 [OPTIONS] COMMAND\n"
        "\n"
        "where COMMAND is one of:\n"
        "  list [collection]    - lists all contacts, optionally specified by collection name\n"
        "  search <search-text> - searches for contacts with details containing search-text\n"
        "  details <ID...>      - lists details for contacts matching the supplied ID list\n"
        "  links <ID...>        - lists links for contacts matching the supplied ID list\n"
        "  delete <ID...>       - removes contacts matching the supplied ID list\n"
        "  dump [ID...]         - displays contact details in debug format\n"
        "  remove-stale-files   - deletes contact file assets that are no longer used by any contacts\n"
        "  collections          - lists all contact collections\n"
        "  help                 - show this command summary\n"
        "  version              - show the application version\n"
        "\n"
        "OPTIONS:\n"
        "  -t                   - use TAB delimiters\n"
        "\n"
        "Output is written to stdout.\n").arg(app));
    ::exit(1);
}

QContactManager *createManager()
{
    QMap<QString, QString> parameters;
    parameters.insert(QString::fromLatin1("mergePresenceChanges"), QString::fromLatin1("false"));
    return new QContactManager(QString::fromLatin1("org.nemomobile.contacts.sqlite"), parameters);
}

QContactManager &manager()
{
    static QContactManager *mgr(createManager());
    return *mgr;
}

const QList<QContactCollection>& managerCollections()
{
    static const QList<QContactCollection> collections = manager().collections();
    return collections;
}

QSet<quint32> parseIds(const QString &ids)
{
    QSet<quint32> rv;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    foreach (const QString &id, ids.split(QChar::fromLatin1(','), Qt::SkipEmptyParts)) {
#else
    foreach (const QString &id, ids.split(QChar::fromLatin1(','), QString::SkipEmptyParts)) {
#endif
        bool ok(false);
        quint32 value(id.toUInt(&ok));
        if (!ok) {
            errorMessage(QString::fromLatin1("Invalid ID value: %1").arg(id));
            ::exit(2);
        }
        rv.insert(value);
    }

    return rv;
}

QSet<quint32> parseIds(char **begin, char **end)
{
    QSet<quint32> rv;

    while (begin != end) {
        rv |= parseIds(QString::fromUtf8(*begin));
        ++begin;
    }

    return rv;
}

template<typename T, typename F>
QContactDetailFilter detailFilter(F field, const QString &searchText, bool partialMatch = false)
{
    QContactDetailFilter filter;
    filter.setDetailType(T::Type, field);
    filter.setValue(searchText);
    if (partialMatch) {
        filter.setMatchFlags(QContactFilter::MatchContains | QContactFilter::MatchFixedString);
    }
    return filter;
}

QContactFilter aggregateContactFilter()
{
    QContactCollectionFilter filter;
    filter.setCollectionId(SeasideCache::aggregateCollectionId());
    return filter;
}

QContactFilter contactIdsFilter(const QSet<quint32> &ids)
{
    QList<QContactId> filterIds;
    foreach (quint32 id, ids) {
        filterIds.append(SeasideCache::apiId(id));
    }

    QContactIdFilter filter;
    filter.setIds(filterIds);
    return filter;
}

quint32 numericId(const QContactId &contactId)
{
    return SeasideCache::contactId(contactId);
}

QString displayLabel(const QContact &contact)
{
    const QChar quote(QChar::fromLatin1('\''));
    return quote + SeasideCache::generateDisplayLabel(contact) + quote;
}

void getRelatedContacts(const QContact &contact, QSet<quint32> *constituents, QSet<quint32> *aggregates)
{
    const quint32 id(numericId(contact.id()));

    foreach (const QContactRelationship &relationship, contact.relationships(QString::fromLatin1("Aggregates"))) {
        const quint32 firstId(numericId(relationship.first()));
        const quint32 secondId(numericId(relationship.second()));
        if ((firstId == id) && constituents) {
            constituents->insert(secondId);
        } else if ((secondId == id) && aggregates) {
            aggregates->insert(firstId);
        }
    }
}

QString printContexts(const QList<int> &contexts)
{
    QStringList list;
    foreach (int context, contexts) {
        if (context == QContactDetail::ContextHome) {
            list.append(QString::fromLatin1("Home"));
        } else if (context == QContactDetail::ContextWork) {
            list.append(QString::fromLatin1("Work"));
        } else if (context == QContactDetail::ContextOther) {
            list.append(QString::fromLatin1("Other"));
        }
    }
    return list.join(QChar::fromLatin1(','));
}

QString printSubTypes(const QList<int> &subTypes)
{
    QStringList list;
    foreach (int subType, subTypes) {
        list.append(QString::number(subType));
    }
    return list.join(QChar::fromLatin1(','));
}

QString print(const QContactAddress &detail)
{
    QStringList list;
    list.append(detail.postOfficeBox());
    list.append(detail.street());
    list.append(detail.locality());
    list.append(detail.region());
    list.append(detail.country());
    list.append(detail.postcode());
    list.append(printSubTypes(detail.subTypes()));
    list.append(printContexts(detail.contexts()));
    return QString::fromLatin1("Address:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactAnniversary &detail)
{
    QStringList list;
    list.append(detail.originalDate().toString(Qt::ISODate));
    list.append(detail.event());
    list.append(detail.calendarId());
    list.append(QString::number(detail.subType()));
    return QString::fromLatin1("Anniversary:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactAvatar &detail)
{
    QStringList list;
    list.append(detail.imageUrl().toString());
    list.append(detail.value(QContactAvatar::FieldMetaData).toString());
    return QString::fromLatin1("Avatar:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactBirthday &detail)
{
    QStringList list;
    list.append(detail.date().toString(Qt::ISODate));
    list.append(detail.dateTime().toString(Qt::ISODate));
    list.append(detail.calendarId());
    return QString::fromLatin1("Birthday:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactEmailAddress &detail)
{
    QStringList list;
    list.append(detail.emailAddress());
    list.append(printContexts(detail.contexts()));
    return QString::fromLatin1("EmailAddress:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactFamily &detail)
{
    QStringList list;
    list.append(detail.spouse());
    list.append(detail.children().join(QChar::fromLatin1(',')));
    return QString::fromLatin1("Family:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactFavorite &detail)
{
    return QString::fromLatin1("Favorite:\t") + QString::number(detail.isFavorite() ? 1 : 0);
}

QString print(const QContactGeoLocation &detail)
{
    QStringList list;
    list.append(QString::number(detail.latitude()));
    list.append(QString::number(detail.longitude()));
    list.append(QString::number(detail.accuracy()));
    list.append(QString::number(detail.altitude()));
    list.append(QString::number(detail.altitudeAccuracy()));
    list.append(QString::number(detail.speed()));
    list.append(QString::number(detail.heading()));
    list.append(detail.timestamp().toString(Qt::ISODate));
    return QString::fromLatin1("GeoLocation:\t%1").arg(detail.label()) + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactGender &detail)
{
    return QString::fromLatin1("Gender:\t") + QString::number(detail.gender());
}

QString print(const QContactGuid &detail)
{
    return QString::fromLatin1("GUID:\t") + detail.guid();
}

QString print(const QContactHobby &detail)
{
    return QString::fromLatin1("Hobby:\t") + detail.hobby();
}

QString print(const QContactName &detail)
{
    QStringList list;
    list.append(detail.prefix());
    list.append(detail.firstName());
    list.append(detail.middleName());
    list.append(detail.lastName());
    list.append(detail.suffix());
    return QString::fromLatin1("Name:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactTimestamp &detail)
{
    QStringList list;
    list.append(detail.lastModified().toString(Qt::ISODate));
    list.append(detail.created().toString(Qt::ISODate));
    return QString::fromLatin1("Timestamp:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactSyncTarget &detail)
{
    return QString::fromLatin1("SyncTarget:\t") + detail.syncTarget();
}

QString print(const QContactCollectionId &collectionId)
{
    QContactCollection matchingCollection;
    for (const QContactCollection &collection : managerCollections()) {
        if (collection.id() == collectionId) {
            matchingCollection = collection;
            break;
        }
    }

    const QString name = matchingCollection.id().isNull()
            ? QString::fromLatin1("(no collection)")
            : matchingCollection.metaData(QContactCollection::KeyName).toString();
    return QString::fromLatin1("Collection:\t") + name;
}

QString print(const QContactNickname &detail)
{
    return QString::fromLatin1("Nickname:\t") + detail.nickname();
}

QString print(const QContactNote &detail)
{
    return QString::fromLatin1("Note:\t") + detail.note();
}

QString print(const QContactOnlineAccount &detail)
{
    QStringList list;
    list.append(detail.accountUri());
    list.append(detail.value(QContactOnlineAccount__FieldAccountPath).toString());
    list.append(detail.serviceProvider());
    list.append(QString::number(detail.protocol()));
    list.append(printContexts(detail.contexts()));
    return QString::fromLatin1("OnlineAccount:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactOrganization &detail)
{
    QStringList list;
    list.append(detail.name());
    list.append(detail.title());
    list.append(detail.role());
    list.append(detail.department());
    list.append(detail.assistantName());
    list.append(detail.logoUrl().toString());
    return QString::fromLatin1("Organization:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactPhoneNumber &detail)
{
    QStringList list;
    list.append(detail.number());
    list.append(printSubTypes(detail.subTypes()));
    list.append(printContexts(detail.contexts()));
    return QString::fromLatin1("PhoneNumber:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactPresence &detail)
{
    QStringList list;
    list.append(QString::number(detail.presenceState()));
    list.append(detail.customMessage());
    list.append(detail.nickname());
    list.append(detail.timestamp().toString(Qt::ISODate));
    list.append(detail.linkedDetailUris().join(QChar::fromLatin1('|')));
    return QString::fromLatin1("Presence:\t") + list.join(QChar::fromLatin1(';'));
}

QString print(const QContactRingtone &detail)
{
    return QString::fromLatin1("Ringtone:\t") + detail.audioRingtoneUrl().toString();
}

QString print(const QContactTag &detail)
{
    return QString::fromLatin1("Tag:\t") + detail.tag();
}

QString print(const QContactUrl &detail)
{
    QStringList list;
    list.append(detail.url());
    list.append(printContexts(detail.contexts()));
    return QString::fromLatin1("URL:\t") + list.join(QChar::fromLatin1(';'));
}

void printContactSummary(const QContact &contact)
{
    QTextStream output(stdout);
    output << " ID:\t" << numericId(contact.id())
           << "\t" << displayLabel(contact)
           << "\t" << print(contact.detail<QContactName>())
           << "\t" << print(contact.collectionId())
           << "\t" << print(contact.detail<QContactSyncTarget>())
           << "\n";
}

void printContactSummary(const QList<QContact> &contacts)
{
    foreach (const QContact &contact, contacts) {
        printContactSummary(contact);
    }
}

void printContactDetails(const QContact &contact)
{
    const quint32 id(numericId(contact.id()));

    QTextStream output(stdout);
    output << " ID:\t" << id
           << delimiter << displayLabel(contact)
           << delimiter << print(contact.detail<QContactName>())
           << delimiter << print(contact.collectionId())
           << delimiter << print(contact.detail<QContactGender>())
           << delimiter << print(contact.detail<QContactFavorite>())
           << delimiter << print(contact.detail<QContactTimestamp>())
           << "\n";

    output << "  Details:";
    foreach (const QContactAddress &detail, contact.details<QContactAddress>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactAnniversary &detail, contact.details<QContactAnniversary>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactAvatar &detail, contact.details<QContactAvatar>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactBirthday &detail, contact.details<QContactBirthday>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactEmailAddress &detail, contact.details<QContactEmailAddress>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactFamily &detail, contact.details<QContactFamily>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactGeoLocation &detail, contact.details<QContactGeoLocation>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactGuid &detail, contact.details<QContactGuid>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactHobby &detail, contact.details<QContactHobby>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactNickname &detail, contact.details<QContactNickname>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactNote &detail, contact.details<QContactNote>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactOnlineAccount &detail, contact.details<QContactOnlineAccount>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactOrganization &detail, contact.details<QContactOrganization>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactPhoneNumber &detail, contact.details<QContactPhoneNumber>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactPresence &detail, contact.details<QContactPresence>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactRingtone &detail, contact.details<QContactRingtone>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactTag &detail, contact.details<QContactTag>()) {
        output << delimiter << print(detail);
    }
    foreach (const QContactUrl &detail, contact.details<QContactUrl>()) {
        output << delimiter << print(detail);
    }
    output << "\n";

    QSet<quint32> constituents;
    QSet<quint32> aggregates;
    getRelatedContacts(contact, &constituents, &aggregates);

    if (!constituents.isEmpty()) {
        QStringList list;
        foreach (quint32 id, constituents) {
            list.append(QString::number(id));
        }
        output << "  Constituents:\t" << list.join(QChar::fromLatin1(' ')) << "\n";
    }
    if (!aggregates.isEmpty()) {
        QStringList list;
        foreach (quint32 id, aggregates) {
            list.append(QString::number(id));
        }
        output << "  Aggregates:\t" << list.join(QChar::fromLatin1(' ')) << "\n";
    }
}

void printContactDetails(const QList<QContact> &contacts)
{
    foreach (const QContact &contact, contacts) {
        printContactDetails(contact);
    }
}

void printContactLinks(const QContact &contact)
{
    const quint32 id(numericId(contact.id()));

    QTextStream output(stdout);
    output << " ID:\t" << id
           << "\t" << displayLabel(contact)
           << "\t" << print(contact.collectionId())
           << "\n";

    QSet<quint32> constituents;
    QSet<quint32> aggregates;
    getRelatedContacts(contact, &constituents, &aggregates);

    if (!constituents.isEmpty()) {
        output << "  Constituents:\n";
        foreach (quint32 id, constituents) {
            QContact constituent(manager().contact(SeasideCache::apiId(id)));
            output << "   ID:\t" << id
                   << "\t" << displayLabel(constituent)
                   << "\t" << print(constituent.collectionId())
                   << "\n";
        }
    }
    if (!aggregates.isEmpty()) {
        output << "  Aggregates:\n";
        foreach (quint32 id, aggregates) {
            QContact aggregate(manager().contact(SeasideCache::apiId(id)));
            output << "   ID:\t" << id
                   << "\t" << displayLabel(aggregate)
                   << "\t" << print(aggregate.collectionId())
                   << "\n";
        }
    }
}

void printContactLinks(const QList<QContact> &contacts)
{
    foreach (const QContact &contact, contacts) {
        printContactLinks(contact);
    }
}

QTextStream &utf8(QTextStream &ts)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts.setCodec("UTF-8");
#endif
    return ts;
}

QString dumpContact(const QContact &contact)
{
    QString rv;
    {
        // Use the debug output operator to dump this contact
        QDebug dbg(&rv);
        dbg << utf8 << contact;
    }
    return rv;
}

void dumpContactDetails(const QContact &contact)
{
    QTextStream output(stdout);
    output << " ID:\t" << numericId(contact.id()) << "\t" << dumpContact(contact) << "\n";

    QSet<quint32> constituents;
    getRelatedContacts(contact, &constituents, 0);

    if (!constituents.isEmpty()) {
        output << "  Constituents:\n";
        foreach (quint32 id, constituents) {
            QContact constituent(manager().contact(SeasideCache::apiId(id)));
            output << "   ID:\t" << id << "\t" << dumpContact(constituent) << "\n";
        }
    }
}

void dumpContactDetails(const QList<QContact> &contacts)
{
    foreach (const QContact &contact, contacts) {
        dumpContactDetails(contact);
    }
}

int listContacts(char **begin, char **end)
{
    if (begin == end) {
        // Show aggregate contacts only
        printContactSummary(manager().contacts(aggregateContactFilter()));
        return 0;
    }

    QString collectionName(QString::fromUtf8(*begin));
    if (++begin != end) {
        errorMessage(QString::fromLatin1("Only one collection is supported"));
        return 3;
    }

    QContactCollectionId matchingCollectionId;
    for (const QContactCollection &collection : managerCollections()) {
        if (collection.metaData(QContactCollection::KeyName).toString() == collectionName) {
            matchingCollectionId = collection.id();
            break;
        }
    }

    if (matchingCollectionId.isNull()) {
        errorMessage(QString::fromLatin1("Cannot find collection named '%1'").arg(collectionName));
        return 3;
    }

    QContactCollectionFilter filter;
    filter.setCollectionId(matchingCollectionId);
    printContactSummary(manager().contacts(filter));

    return 0;
}

int searchContacts(char **begin, char **end)
{
    if (begin == end) {
        errorMessage(QString::fromLatin1("Search text required"));
        return 2;
    }

    QString searchText(QString::fromUtf8(*begin));
    if (++begin != end) {
        errorMessage(QString::fromLatin1("Only one search text item is supported"));
        return 3;
    }
    
    // Match this text input to anything reasonable
    QContactFilter filter = detailFilter<QContactName>(QContactName::FieldPrefix, searchText, true) |
                            detailFilter<QContactName>(QContactName::FieldFirstName, searchText, true) |
                            detailFilter<QContactName>(QContactName::FieldMiddleName, searchText, true) |
                            detailFilter<QContactName>(QContactName::FieldLastName, searchText, true) |
                            detailFilter<QContactName>(QContactName::FieldSuffix, searchText, true) |
                            detailFilter<QContactAddress>(QContactAddress::FieldPostOfficeBox, searchText, true) |
                            detailFilter<QContactAddress>(QContactAddress::FieldStreet, searchText, true) |
                            detailFilter<QContactAddress>(QContactAddress::FieldLocality, searchText, true) |
                            detailFilter<QContactAddress>(QContactAddress::FieldRegion, searchText, true) |
                            detailFilter<QContactAddress>(QContactAddress::FieldCountry, searchText, true) |
                            detailFilter<QContactAddress>(QContactAddress::FieldPostcode, searchText, true) |
                            detailFilter<QContactEmailAddress>(QContactEmailAddress::FieldEmailAddress, searchText, true) |
                            detailFilter<QContactGuid>(QContactGuid::FieldGuid, searchText, true) |
                            detailFilter<QContactHobby>(QContactHobby::FieldHobby, searchText, true) |
                            detailFilter<QContactNickname>(QContactNickname::FieldNickname, searchText, true) |
                            detailFilter<QContactNote>(QContactNote::FieldNote, searchText, true) |
                            detailFilter<QContactOnlineAccount>(QContactOnlineAccount::FieldAccountUri, searchText, true) |
                            detailFilter<QContactOrganization>(QContactOrganization::FieldName, searchText, true) |
                            detailFilter<QContactOrganization>(QContactOrganization::FieldTitle, searchText, true) |
                            detailFilter<QContactOrganization>(QContactOrganization::FieldRole, searchText, true) |
                            detailFilter<QContactOrganization>(QContactOrganization::FieldDepartment, searchText, true) |
                            detailFilter<QContactPhoneNumber>(QContactPhoneNumber::FieldNumber, searchText, true) |
                            detailFilter<QContactPresence>(QContactPresence::FieldNickname, searchText, true) |
                            detailFilter<QContactTag>(QContactTag::FieldTag, searchText, true) |
                            detailFilter<QContactUrl>(QContactUrl::FieldUrl, searchText, true);

    printContactSummary(manager().contacts(filter));
    return 0;
}

int showDetails(char **begin, char **end)
{
    if (begin == end) {
        errorMessage(QString::fromLatin1("ID list required"));
        return 2;
    }

    QSet<quint32> contactIds(parseIds(begin, end));
    printContactDetails(manager().contacts(contactIdsFilter(contactIds)));
    return 0;
}

int showLinks(char **begin, char **end)
{
    if (begin == end) {
        errorMessage(QString::fromLatin1("ID list required"));
        return 2;
    }

    QSet<quint32> contactIds(parseIds(begin, end));
    printContactLinks(manager().contacts(contactIdsFilter(contactIds)));
    return 0;
}

int deleteContacts(char **begin, char **end)
{
    if (begin == end) {
        errorMessage(QString::fromLatin1("ID list required"));
        return 2;
    }

    QTextStream output(stdout);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    output << QString::fromLatin1("\nSelected contacts:\n") << Qt::flush;
#else
    output << QString::fromLatin1("\nSelected contacts:\n") << flush;
#endif
    QSet<quint32> contactIds(parseIds(begin, end));
    printContactSummary(manager().contacts(contactIdsFilter(contactIds)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    output << QString::fromLatin1("\nAre you sure you want to delete these contacts? (y/N):") << Qt::flush;
#else
    output << QString::fromLatin1("\nAre you sure you want to delete these contacts? (y/N):") << flush;
#endif
    QString confirmation;
    QTextStream(stdin) >> confirmation;
    if (confirmation == QString::fromLatin1("y")) {
        QList<QContactId> removeIds;
        foreach (quint32 id, contactIds) {
            removeIds.append(SeasideCache::apiId(id));
        }

        return manager().removeContacts(removeIds) ? 0 : 3;
    }

    output << "\nDeletion aborted\n";
    return 0;
}

int dumpContacts(char **begin, char **end)
{
    QSet<quint32> contactIds;

    if (begin != end) {
        contactIds = parseIds(begin, end);
    } else {
        // Dump all aggregate IDs
        foreach (const QContactId &id, manager().contactIds(aggregateContactFilter())) {
            contactIds.insert(SeasideCache::internalId(id));
        }
    }

    dumpContactDetails(manager().contacts(contactIdsFilter(contactIds)));
    return 0;
}

int dumpCollections()
{
    QTextStream output(stdout);

    if (managerCollections().isEmpty()) {
        output << "No collections found\n";
        return 0;
    }

    for (const QContactCollection &collection : managerCollections()) {
        output << "  ID: " << collection.id().toString()
               << "  Name: " << collection.metaData(QContactCollection::KeyName).toString()
               << "\n";
    }

    return 0;
}

QString localFilePathWithoutScheme(const QString &filePath)
{
    QString path;
    QUrl url(filePath);
    if (url.isLocalFile()) {
        path = url.toLocalFile();
    } else {
        path = filePath;
    }
    return path.startsWith('/')
            ? path.replace(QLatin1String("//"), QLatin1String("/"))    // remove redundant chars
            : QString();
}

void removeStaleAvatars(QContactManager *manager, const QString &avatarDirPath)
{
    QContactFetchHint fetchHint;
    fetchHint.setOptimizationHints(QContactFetchHint::NoRelationships);
    fetchHint.setDetailTypesHint(QList<QContactDetail::DetailType>()
                                 << QContactDetail::TypeAvatar);
    QContactDetailFilter hasAvatarFilter;
    hasAvatarFilter.setDetailType(QContactDetail::TypeAvatar);

    QSet<QString> avatarImageUrls;
    const QList<QContact> contacts = manager->contacts(hasAvatarFilter, QList<QContactSortOrder>(), fetchHint);
    for (const QContact &contact : contacts) {
        const QList<QContactAvatar> avatars = contact.details<QContactAvatar>();
        for (const QContactAvatar &avatar : avatars) {
            const QString localFilePath = localFilePathWithoutScheme(avatar.imageUrl().toString());
            if (!localFilePath.isEmpty()) {
                avatarImageUrls.insert(localFilePath);
            }
        }
    }

    QDir avatarDir(avatarDirPath);
    QStringList removedFiles;
    const QStringList avatarFileNames = avatarDir.entryList(QDir::Files);
    for (const QString &avatarFileName : avatarFileNames) {
        const QString avatarFilePath = localFilePathWithoutScheme(avatarDir.absoluteFilePath(avatarFileName));
        if (!avatarImageUrls.remove(avatarFilePath)) {
            removedFiles.append(avatarFilePath);
            QFile::remove(avatarFilePath);
        }
    }

    QTextStream output(stdout);
    if (removedFiles.count()) {
        output << "Removed unused avatars:\n  " << removedFiles.join("\n  ") << "\n";
    } else {
        output << "Nothing to remove, no unused avatars found in " << avatarDirPath << "\n";
    }
}

int removeStaleFiles()
{
    const QString dataPath = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).value(0);

    // For non-privileged contact manager
    QMap<QString, QString> parameters;
    parameters.insert(QString::fromLatin1("nonprivileged"), QString::fromLatin1("true"));
    QContactManager mgr(QString::fromLatin1("org.nemomobile.contacts.sqlite"), parameters);
    removeStaleAvatars(&mgr, QString("%1/system/Contacts/avatars").arg(dataPath));

    // For privileged contact manager
    removeStaleAvatars(&manager(), QString("%1/data/avatars").arg(dataPath));

    return 0;
}

}

int main(int argc, char **argv)
{
    QCoreApplication qca(argc, argv);

    const QString app(QString::fromLatin1(argv[0]));

    if (argc <= 1) {
        invalidUsage(app);
    }

    int offset = 2;

    QString command(QString::fromLatin1(argv[1]));
    if (command.at(0) == QChar::fromLatin1('-')) {
        if (command == QString::fromLatin1("-t")) {
            // Use tab as a delimiter (better for CSV/cut)
            delimiter = QString::fromLatin1("\t");

            command = QString::fromLatin1(argv[2]);
            ++offset;
        } else {
            errorMessage(QString::fromLatin1("Invalid option: %1").arg(command));
            invalidUsage(app);
        }
    }
    
    char **remaining = &argv[offset];
    char **end = &argv[argc];

    if (command == QString::fromLatin1("list")) {
        return listContacts(remaining, end);
    }
    if (command == QString::fromLatin1("search")) {
        return searchContacts(remaining, end);
    }
    if (command == QString::fromLatin1("details")) {
        return showDetails(remaining, end);
    }
    if (command == QString::fromLatin1("links")) {
        return showLinks(remaining, end);
    }
    if (command == QString::fromLatin1("delete")) {
        return deleteContacts(remaining, end);
    }
    if (command == QString::fromLatin1("dump")) {
        return dumpContacts(remaining, end);
    }
    if (command == QString::fromLatin1("remove-stale-files")) {
        return removeStaleFiles();
    }
    if (command == QString::fromLatin1("collections")) {
        return dumpCollections();
    }

    if (command == QString::fromLatin1("version")) {
        errorMessage(QString::fromLatin1("%1 Version: " VERSION_STRING).arg(app));
        return 0;
    }

    if (command != QString::fromLatin1("help")) {
        errorMessage(QString::fromLatin1("Invalid command: %1").arg(command));
    }
    invalidUsage(app);
}

