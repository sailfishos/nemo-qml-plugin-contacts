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

#include "seasideimport.h"

#include <QContactIdFilter>
#include <QContact>
#include <QContactManager>

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
}

QList<QContact> SeasideImport::buildImportContacts(
        const QList<QVersitDocument> &details,
        int *newCount,
        int *updatedCount,
        int *ignoredCount,
        SeasideContactBuilder *contactBuilder,
        bool skipLocalDupDetection)
{
    if (newCount)
        *newCount = 0;
    if (updatedCount)
        *updatedCount = 0;
    int existingCount = 0;
    bool eraseMatch = false;

    SeasideContactBuilder *builder = contactBuilder
                                   ? contactBuilder
                                   : new SeasideContactBuilder;
    QList<QContact> importedContacts = builder->importContacts(details);

    // Preprocess the imported contacts and merge any duplicates in the import list
    QList<QContact>::iterator it = importedContacts.begin();
    while (it != importedContacts.end()) {
        builder->preprocessContact(*it);
        int previousIndex = builder->previousDuplicateIndex(importedContacts, it - importedContacts.begin());
        if (previousIndex != -1) {
            // Combine these duplicate contacts
            QContact &previous(importedContacts[previousIndex]);
            builder->mergeImportIntoImport(previous, *it, &eraseMatch);
            if (eraseMatch) {
                it = importedContacts.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    if (!skipLocalDupDetection) {
        // Build up information about local device contacts, so we can detect matches
        // in order to correctly set the appropriate ContactId in the imported contacts
        // prior to save (thereby ensuring correct add vs update save semantics).
        builder->buildLocalDeviceContactIndexes();

        // Find any imported contacts that match contacts we already have
        QMap<QContactId, int> existingIds;
        it = importedContacts.begin();
        while (it != importedContacts.end()) {
            QContactId existingId = builder->matchingLocalContactId(*it);
            if (!existingId.isNull()) {
                QMap<QContactId, int>::iterator eit = existingIds.find(existingId);
                if (eit == existingIds.end()) {
                    // this match hasn't been seen before.
                    existingIds.insert(existingId, (it - importedContacts.begin()));
                    ++it;
                } else {
                    // another import contact which matches that local contact has
                    // been seen already. Merge these both-matching import contacts.
                    QContact &previous(importedContacts[*eit]);
                    builder->mergeImportIntoImport(previous, *it, &eraseMatch);
                    if (eraseMatch) {
                        it = importedContacts.erase(it);
                    } else {
                        ++it;
                    }
                }
            } else {
                ++it;
            }
        }

        existingCount = existingIds.count();
        if (existingCount > 0) {
            // Retrieve all the contacts that we have matches for
            QContactIdFilter idFilter;
            idFilter.setIds(existingIds.keys());

            QSet<QContactId> modifiedContacts;
            QSet<QContactId> unmodifiedContacts;
            QHash<QContactId, bool> unmodifiedErase;

            foreach (const QContact &contact, builder->manager()->contacts(idFilter & builder->mergeSubsetFilter(), QList<QContactSortOrder>(), basicFetchHint())) {
                QMap<QContactId, int>::const_iterator it = existingIds.find(contact.id());
                if (it != existingIds.end()) {
                    // Update the existing version of the contact with any new details
                    QContact &importContact(importedContacts[*it]);
                    bool modified = builder->mergeLocalIntoImport(importContact, contact, &eraseMatch);
                    if (modified) {
                        modifiedContacts.insert(importContact.id());
                    } else {
                        unmodifiedContacts.insert(importContact.id());
                        unmodifiedErase.insert(importContact.id(), eraseMatch);
                    }
                } else {
                    qWarning() << "unable to update existing contact:" << contact.id();
                }
            }

            if (!unmodifiedContacts.isEmpty()) {
                QList<QContact>::iterator it = importedContacts.begin();
                while (it != importedContacts.end()) {
                    const QContact &importContact(*it);
                    const QContactId contactId(importContact.id());

                    if (!modifiedContacts.contains(contactId) && unmodifiedContacts.contains(contactId) && unmodifiedErase.value(contactId, false) == true) {
                        // This contact was not modified by import and should be erased from the import list - don't update it
                        it = importedContacts.erase(it);
                        --existingCount;
                    } else {
                        ++it;
                    }
                }
            }
        }
    }

    if (updatedCount)
        *updatedCount = existingCount;
    if (newCount)
        *newCount = importedContacts.count() - existingCount;
    if (ignoredCount) // duplicates or insignificant updates
        *ignoredCount = details.count() - importedContacts.count();

    return importedContacts;
}

