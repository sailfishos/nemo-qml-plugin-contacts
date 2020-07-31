/*
 * Copyright (c) 2015 - 2020 Jolla Ltd.
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

#include "seasidecontactbuilder.h"

#include "seasidecache.h"
#include "seasidepropertyhandler.h"

#include <QContactDetailFilter>
#include <QContactFetchHint>
#include <QContactManager>
#include <QContactSortOrder>
#include <QContactSyncTarget>

#include <QContactAddress>
#include <QContactAnniversary>
#include <QContactAvatar>
#include <QContactBirthday>
#include <QContactEmailAddress>
#include <QContactFamily>
#include <QContactGeoLocation>
#include <QContactGuid>
#include <QContactHobby>
#include <QContactName>
#include <QContactNickname>
#include <QContactNote>
#include <QContactOnlineAccount>
#include <QContactOrganization>
#include <QContactPhoneNumber>
#include <QContactRingtone>
#include <QContactTag>
#include <QContactUrl>

#include <QContactIdFilter>
#include <QContactExtendedDetail>

#include <QVersitContactExporter>
#include <QVersitContactImporter>
#include <QVersitReader>
#include <QVersitWriter>

#include <QHash>
#include <QString>
#include <QList>
#include <QDebug>

namespace {

QContactFetchHint basicFetchHint()
{
    QContactFetchHint fetchHint;

    fetchHint.setOptimizationHints(QContactFetchHint::NoRelationships |
                                   QContactFetchHint::NoActionPreferences |
                                   QContactFetchHint::NoBinaryBlobs);

    return fetchHint;
}

QContactFilter localContactFilter()
{
    // Contacts that are local to the device have sync target 'local' or 'was_local' or 'bluetooth'
    QContactDetailFilter filterLocal, filterWasLocal, filterBluetooth;
    filterLocal.setDetailType(QContactSyncTarget::Type, QContactSyncTarget::FieldSyncTarget);
    filterWasLocal.setDetailType(QContactSyncTarget::Type, QContactSyncTarget::FieldSyncTarget);
    filterBluetooth.setDetailType(QContactSyncTarget::Type, QContactSyncTarget::FieldSyncTarget);
    filterLocal.setValue(QString::fromLatin1("local"));
    filterWasLocal.setValue(QString::fromLatin1("was_local"));
    filterBluetooth.setValue(QString::fromLatin1("bluetooth"));

    return filterLocal | filterWasLocal | filterBluetooth;
}

bool allCharactersMatchScript(const QString &s, QChar::Script script)
{
    for (QString::const_iterator it = s.constBegin(), end = s.constEnd(); it != end; ++it) {
        if ((*it).script() != script) {
            return false;
        }
    }

    return true;
}

bool applyNameFixes(QContactName *nameDetail)
{
    // Chinese names shouldn't have a middle name, so if it is present in a Han-script-only
    // name, it is probably wrong and it should be prepended to the first name instead.
    QString middleName = nameDetail->middleName();
    if (middleName.isEmpty()) {
        return false;
    }

    QString firstName = nameDetail->firstName();
    QString lastName = nameDetail->lastName();
    if (!allCharactersMatchScript(middleName, QChar::Script_Han)
            || (!firstName.isEmpty() && !allCharactersMatchScript(firstName, QChar::Script_Han))
            || (!lastName.isEmpty() && !allCharactersMatchScript(lastName, QChar::Script_Han))) {
        return false;
    }

    nameDetail->setFirstName(middleName + firstName);
    nameDetail->setMiddleName(QString());
    return true;
}

bool nameIsEmpty(const QContactName &name)
{
    if (name.isEmpty())
        return true;

    return (name.prefix().isEmpty() &&
            name.firstName().isEmpty() &&
            name.middleName().isEmpty() &&
            name.lastName().isEmpty() &&
            name.suffix().isEmpty());
}

QString contactNameString(const QContact &contact)
{
    QStringList details;
    QContactName name(contact.detail<QContactName>());
    if (nameIsEmpty(name))
        return QString();

    details.append(name.prefix());
    details.append(name.firstName());
    details.append(name.middleName());
    details.append(name.lastName());
    details.append(name.suffix());
    return details.join(QChar::fromLatin1('|'));
}

void setNickname(QContact &contact, const QString &text)
{
    foreach (const QContactNickname &nick, contact.details<QContactNickname>()) {
        if (nick.nickname() == text) {
            return;
        }
    }

    QContactNickname nick;
    nick.setNickname(text);
    contact.saveDetail(&nick);
}


template<typename T, typename F>
QVariant detailValue(const T &detail, F field)
{
    return detail.value(field);
}

typedef QMap<int, QVariant> DetailMap;

DetailMap detailValues(const QContactDetail &detail)
{
    DetailMap rv(detail.values());
    return rv;
}

static bool variantEqual(const QVariant &lhs, const QVariant &rhs)
{
    // Work around incorrect result from QVariant::operator== when variants contain QList<int>
    static const int QListIntType = QMetaType::type("QList<int>");

    const int lhsType = lhs.userType();
    if (lhsType != rhs.userType()) {
        return false;
    }

    if (lhsType == QListIntType) {
        return (lhs.value<QList<int> >() == rhs.value<QList<int> >());
    }
    return (lhs == rhs);
}

static bool detailValuesSuperset(const QContactDetail &lhs, const QContactDetail &rhs)
{
    // True if all values in rhs are present in lhs
    const DetailMap lhsValues(detailValues(lhs));
    const DetailMap rhsValues(detailValues(rhs));

    if (lhsValues.count() < rhsValues.count()) {
        return false;
    }

    foreach (const DetailMap::key_type &key, rhsValues.keys()) {
        if (!variantEqual(lhsValues[key], rhsValues[key])) {
            return false;
        }
    }

    return true;
}

static void fixupDetail(QContactDetail &)
{
}

// Fixup QContactUrl because importer produces incorrectly typed URL field
static void fixupDetail(QContactUrl &url)
{
    QVariant urlField = url.value(QContactUrl::FieldUrl);
    if (!urlField.isNull()) {
        QString urlString = urlField.toString();
        if (!urlString.isEmpty()) {
            url.setValue(QContactUrl::FieldUrl, QUrl(urlString));
        } else {
            url.setValue(QContactUrl::FieldUrl, QVariant());
        }
    }
}

// Fixup QContactOrganization because importer produces invalid department
static void fixupDetail(QContactOrganization &org)
{
    QVariant deptField = org.value(QContactOrganization::FieldDepartment);
    if (!deptField.isNull()) {
        QStringList deptList = deptField.toStringList();

        // Remove any empty elements from the list
        QStringList::iterator it = deptList.begin();
        while (it != deptList.end()) {
            if ((*it).isEmpty()) {
                it = deptList.erase(it);
            } else {
                ++it;
            }
        }

        if (!deptList.isEmpty()) {
            org.setValue(QContactOrganization::FieldDepartment, deptList);
        } else {
            org.setValue(QContactOrganization::FieldDepartment, QVariant());
        }
    }
}

template<typename T>
bool mergeContactDetails(QContact *mergeInto, const QContact &mergeFrom, bool singular = false)
{
    bool rv = false;

    QList<T> existingDetails(mergeInto->details<T>());
    if (singular && !existingDetails.isEmpty())
        return rv;

    foreach (T detail, mergeFrom.details<T>()) {
        // Make any corrections to the input
        fixupDetail(detail);

        // See if the contact already has a detail which is a superset of this one
        bool found = false;
        foreach (const T &existing, existingDetails) {
            if (detailValuesSuperset(existing, detail)) {
                found = true;
                break;
            }
        }
        if (!found) {
            mergeInto->saveDetail(&detail);
            rv = true;
        }
    }

    return rv;
}

bool mergeContacts(QContact *mergeInto, const QContact &mergeFrom)
{
    bool rv = false;

    // Update the existing contact with any details in the new import
    rv |= mergeContactDetails<QContactAddress>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactAnniversary>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactAvatar>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactBirthday>(mergeInto, mergeFrom, true);
    rv |= mergeContactDetails<QContactEmailAddress>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactFamily>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactGeoLocation>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactGuid>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactHobby>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactNickname>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactNote>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactOnlineAccount>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactOrganization>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactPhoneNumber>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactRingtone>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactTag>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactUrl>(mergeInto, mergeFrom);
    rv |= mergeContactDetails<QContactExtendedDetail>(mergeInto, mergeFrom);

    return rv;
}

}

SeasideContactBuilder::SeasideContactBuilder()
    : d(new SeasideContactBuilderPrivate)
{
    // defaults.  override in the ctor of your derived type.
    d->manager = 0;
    d->propertyHandler = 0;
    d->unimportableDetailTypes = (QSet<QContactDetail::DetailType>() << QContactDetail::TypeGlobalPresence << QContactDetail::TypeVersion);
    d->importableSyncTargets = (QStringList() << QLatin1String("was_local") << QLatin1String("bluetooth"));
}

SeasideContactBuilder::~SeasideContactBuilder()
{
    delete d->propertyHandler;
    delete d;
}

/*
 * Returns a pointer to a valid QContactManager.
 * The default implementation uses the SeasideCache manager.
 */
QContactManager *SeasideContactBuilder::manager()
{
    if (!d->manager) {
        d->manager = SeasideCache::manager();
    }

    return d->manager;
}

/*
 * Returns a filter which will return the subset of contacts
 * in the manager which are potential merge candidates for
 * the imported contacts (ie, come from the same sync target).
 *
 * The default implementation will return a filter which matches
 * any local / was_local / Bluetooth contact.
 */
QContactFilter SeasideContactBuilder::mergeSubsetFilter() const
{
    return localContactFilter();
}

/*
 * Returns a versit property handler which will be used during
 * conversion of versit documents into QContact instances.
 *
 * The default implementation will return a SeasidePropertyHandler.
 */
QVersitContactHandler *SeasideContactBuilder::propertyHandler()
{
    if (!d->propertyHandler) {
        d->propertyHandler = new SeasidePropertyHandler;
    }

    return d->propertyHandler;
}

/*
 * Merge the given (matching) \a local contact into the given
 * \a import contact, so that the \a import contact could be
 * later saved in the manager.
 *
 * Returns \c true if the \a import contact differed significantly
 * from the \a local contact (that is, if saving the returned
 * \a import contact would result in the \a local contact being
 * updated).
 *
 * The \a erase value will be set to true if the given import
 * contact should be erased from the imported contacts list if
 * there are no significant differences between it and the local
 * contact (that is, if the imported contact should be omitted
 * from later possible store operations, due to the fact that such
 * a store operation would be a no-op for that contact).
 *
 * The default implementation performs a non-destructive merge
 * and will set \a erase to true, which is the required behaviour
 * for an "import" style sync.
 *
 * Sync implementations which require remote-detail-removal
 * semantics, for example, should implement this function
 * differently (eg, prefer-local or prefer-remote), and should
 * possibly set \a erase to false if they don't wish to prune
 * the import list prior to save.
 */
bool SeasideContactBuilder::mergeLocalIntoImport(QContact &import, const QContact &local, bool *erase)
{
    // this implementation does a (mostly) non-destructive detail-addition-merge.
    // other implementations may choose to prefer-local or prefer-remote.
    *erase = true;
    QContact temp(import);
    import = local;
    return mergeContacts(&import, temp);
}

/*
 * Merge the given (matching) \a otherImport contact into the given
 * \a import contact.  This function is set \a erase to true if
 * the \a otherImport contact should be erased from the import list.
 * The function will return \c true if the \a import contact
 * is modified as a result of the merge.
 *
 * Returns \c true if the \a otherImport contact
 * should be erased from the import list, otherwise \c false.
 *
 * The default implementation performs a non-destructive merge
 * into the \a import contact, and then sets \a erase to true, which
 * is the required behaviour for an "import" style sync.
 *
 * Sync implementations which require one-to-one mapping between
 * import contacts and device-stored contacts should set \a erase
 * to false in this function.
 */
bool SeasideContactBuilder::mergeImportIntoImport(QContact &import, QContact &otherImport, bool *erase)
{
    *erase = true;
    return mergeContacts(&import, otherImport);
}

/*
 * Import the given Versit \a documents as QContacts and return them.
 * The default implementation uses a SeasidePropertyHandler during import
 */
QList<QContact> SeasideContactBuilder::importContacts(const QList<QVersitDocument> &documents)
{
    QVersitContactHandler *handler = propertyHandler();
    QVersitContactImporter importer;
    importer.setPropertyHandler(handler);
    importer.importDocuments(documents);
    return importer.contacts();
}

/*
 * Preprocess the given import contact prior to duplicate detection,
 * merging, and subsequent storage.
 *
 * The default implementation performs some fixes for common issues
 * encountered in NAME field details, and removes various detail types
 * which are not supported by the qtcontacts-sqlite manager engine.
 */
void SeasideContactBuilder::preprocessContact(QContact &contact)
{
    // Fix up name (field ordering) if required
    QContactName nameDetail = contact.detail<QContactName>();
    if (applyNameFixes(&nameDetail)) {
        contact.saveDetail(&nameDetail);
    }

    // Remove any details that our backend can't store, or which
    // the client wishes stripped from the imported contacts.
    foreach (QContactDetail detail, contact.details()) {
        if (d->unimportableDetailTypes.contains(detail.type())) {
            qDebug() << "  Removing unimportable detail:" << detail;
            contact.removeDetail(&detail);
        } else if (detail.type() == QContactSyncTarget::Type) {
            // We allow some syncTarget values
            const QString syncTarget(detail.value<QString>(QContactSyncTarget::FieldSyncTarget));
            if (!d->importableSyncTargets.contains(syncTarget)) {
                qDebug() << "  Removing unimportable syncTarget:" << syncTarget;
                contact.removeDetail(&detail);
            }
        }
    }

    // Set nickname by default if the name is empty
    if (contactNameString(contact).isEmpty()) {
        QContactName nameDetail = contact.detail<QContactName>();
        contact.removeDetail(&nameDetail);
        if (contact.details<QContactNickname>().isEmpty()) {
            QString label = contact.detail<QContactDisplayLabel>().label();
            if (label.isEmpty()) {
                label = SeasideCache::generateDisplayLabelFromNonNameDetails(contact);
            }
            setNickname(contact, label);
        }
    }
}

/*
 * Returns the index into the \a importedContacts list at which a
 * duplicate (merge candidate) of the contact at the given
 * \a contactIndex may be found, or \c -1 if no match is found.
 *
 * The default implementation uses a combination of GUID and
 * name / label matching to determine if a contact is duplicated
 * within the import list.
 *
 * The result will be used to merge any duplicated contacts within
 * the import list, which is the required behaviour when performing
 * an "import" style sync.  Any implementation which requires
 * a one-to-one mapping between import contacts and stored device
 * contacts should instead return -1 from this function.
 */
int SeasideContactBuilder::previousDuplicateIndex(QList<QContact> &importedContacts, int contactIndex)
{
    QContact &contact(importedContacts[contactIndex]);
    const QString guid = contact.detail<QContactGuid>().guid();
    const QString name = contactNameString(contact);
    const bool emptyName = name.isEmpty();
    const QString label = contact.detail<QContactDisplayLabel>().label().isEmpty()
                        ?SeasideCache::generateDisplayLabelFromNonNameDetails(contact)
                        : contact.detail<QContactDisplayLabel>().label();

    int previousIndex = -1;
    QHash<QString, int>::const_iterator git = d->importGuids.find(guid);
    if (git != d->importGuids.end()) {
        previousIndex = git.value();

        if (!emptyName) {
            // If we have a GUID match, but names differ, ignore the match
            const QContact &previous(importedContacts[previousIndex]);
            const QString previousName = contactNameString(previous);
            if (!previousName.isEmpty() && (previousName != name)) {
                previousIndex = -1;

                // Remove the conflicting GUID from this contact
                QContactGuid guidDetail = contact.detail<QContactGuid>();
                contact.removeDetail(&guidDetail);
            }
        }
    }
    if (previousIndex == -1) {
        if (!emptyName) {
            QHash<QString, int>::const_iterator nit = d->importNames.find(name);
            if (nit != d->importNames.end()) {
                previousIndex = nit.value();
            }
        } else if (!label.isEmpty()) {
            // Only if name is empty, use displayLabel - probably SIM import
            QHash<QString, int>::const_iterator lit = d->importLabels.find(label);
            if (lit != d->importLabels.end()) {
                previousIndex = lit.value();
            }
        }
    }

    if (previousIndex == -1) {
        // No previous duplicate detected.  This is a new contact.
        // Update our identification hashes with this contact's info.
        if (!guid.isEmpty()) {
            d->importGuids.insert(guid, contactIndex);
        }
        if (!emptyName) {
            d->importNames.insert(name, contactIndex);
        } else if (!label.isEmpty()) {
            d->importLabels.insert(label, contactIndex);
        }
    }

    return previousIndex;
}

/*
 * Build up any indexes of information required to later determine
 * whether a given import contact is already represented on the
 * device (ie, if the "new" contact is actually a new contact,
 * or if it constitutes an update to a previously imported contact).
 *
 * The default implementation builds up hashes of GUID, name and
 * label information to use later during match detection.
 */
void SeasideContactBuilder::buildLocalDeviceContactIndexes()
{
    // Find all names and GUIDs for local contacts that might match these contacts
    QContactFetchHint fetchHint(basicFetchHint());
    fetchHint.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactName::Type << QContactNickname::Type << QContactGuid::Type);

    QContactManager *mgr(manager());

    foreach (const QContact &contact, mgr->contacts(mergeSubsetFilter(), QList<QContactSortOrder>(), fetchHint)) {
        const QString guid = contact.detail<QContactGuid>().guid();
        const QString name = contactNameString(contact);

        if (!guid.isEmpty()) {
            d->existingGuids.insert(guid, contact.id());
        }
        if (!name.isEmpty()) {
            d->existingNames.insert(name, contact.id());
            d->existingContactNames.insert(contact.id(), name);
        }
        foreach (const QContactNickname &nick, contact.details<QContactNickname>()) {
            d->existingNicknames.insert(nick.nickname(), contact.id());
        }
    }
}

/*
 * Returns the id of an existing local device contact which matches
 * the given import \a contact.  This local device contact is a
 * previously-imported version of the "new" import \a contact.
 *
 * The default implementation uses the previously cached GUID,
 * name and label information to perform the match detection.
 */
QContactId SeasideContactBuilder::matchingLocalContactId(QContact &contact)
{
    const QString guid = contact.detail<QContactGuid>().guid();
    const QString name = contactNameString(contact);
    const bool emptyName = name.isEmpty();
    QContactId existingId;

    QHash<QString, QContactId>::const_iterator git = d->existingGuids.find(guid);
    if (git != d->existingGuids.end()) {
        existingId = *git;

        if (!emptyName) {
            // If we have a GUID match, but names differ, ignore the match
            QMap<QContactId, QString>::iterator nit = d->existingContactNames.find(existingId);
            if (nit != d->existingContactNames.end()) {
                const QString &existingName(*nit);
                if (!existingName.isEmpty() && (existingName != name)) {
                    existingId = QContactId();

                    // Remove the conflicting GUID from this contact
                    QContactGuid guidDetail = contact.detail<QContactGuid>();
                    contact.removeDetail(&guidDetail);
                }
            }
        }
    }
    if (existingId.isNull()) {
        if (!emptyName) {
            QHash<QString, QContactId>::const_iterator nit = d->existingNames.find(name);
            if (nit != d->existingNames.end()) {
                existingId = *nit;
            }
        } else {
            foreach (const QContactNickname nick, contact.details<QContactNickname>()) {
                const QString nickname(nick.nickname());
                if (!nickname.isEmpty()) {
                    QHash<QString, QContactId>::const_iterator nit = d->existingNicknames.find(nickname);
                    if (nit != d->existingNicknames.end()) {
                        existingId = *nit;
                        break;
                    }
                }
            }
        }
    }

    return existingId;
}
