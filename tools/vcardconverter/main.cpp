/*
 * Copyright (c) 2012 Robin Burchell <robin+mer@viroteck.net>
 * Copyright (c) 2012 - 2020 Jolla Ltd.
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

#include <qtcontacts-extensions.h>
#include <qtcontacts-extensions_impl.h>

#include <seasideimport.h>
#include <seasideexport.h>

// Qt
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDebug>

// Contacts
#include <QContactCollectionFilter>
#include <QContactManager>
#include <QContactSyncTarget>
#include <QContactDisplayLabel>

// Versit
#include <QVersitReader>
#include <QVersitWriter>

QTCONTACTS_USE_NAMESPACE
QTVERSIT_USE_NAMESPACE

namespace {

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
    errorMessage(QString::fromLatin1("Usage: %1 [-e | --export] <filename> [<collectionId>]").arg(app));
    ::exit(1);
}

QContactFilter collectionFilter(const QContactCollectionId &collectionId)
{
    QContactCollectionFilter filter;
    filter.setCollectionId(collectionId);
    return filter;
}

}

int main(int argc, char **argv)
{
    QCoreApplication qca(argc, argv);

    bool import = true;
    QString filename;
    QString collection;

    const QString app(QString::fromLatin1(argv[0]));

    for (int i = 1; i < argc; ++i) {
        const QString arg(QString::fromLatin1(argv[i]));
        if (arg.startsWith('-')) {
            if (!filename.isNull()) {
                invalidUsage(app);
            } else if (arg == QString::fromLatin1("-e") || arg == QString::fromLatin1("--export")) {
                import = false;
            } else {
                errorMessage(QString::fromLatin1("%1: unknown option: '%2'").arg(app).arg(arg));
                invalidUsage(app);
            }
        } else {
            if (filename.isEmpty()) {
                filename = arg;
            } else {
                collection = arg;
            }
        }
    }

    if (filename.isNull()) {
        errorMessage(QString::fromLatin1("%1: filename must be specified").arg(app));
        invalidUsage(app);
    }

    QFile vcf(filename);
    QIODevice::OpenMode mode(import ? QIODevice::ReadOnly : QIODevice::WriteOnly | QIODevice::Truncate);
    if (!vcf.open(mode)) {
        errorMessage(QString::fromLatin1("%1: file cannot be opened: '%2'").arg(app).arg(filename));
        ::exit(2);
    }

    QContactManager mgr(QString::fromLatin1("org.nemomobile.contacts.sqlite"));
    const QContactCollectionId collectionId(collection.isEmpty() ? QtContactsSqliteExtensions::localCollectionId(mgr.managerUri())
                                                                 : QContactCollectionId::fromString(collection));
    const bool collectionIsLocalAddressbook = collectionId == QtContactsSqliteExtensions::localCollectionId(mgr.managerUri());

    if (!collectionIsLocalAddressbook) {
        if (import) {
            qDebug("Importing contact data to non-local addressbook - this data may be "
                   "synced to the remote account provider or application which owns "
                   "the collection!");
        } else {
            qDebug("Exporting non-local contacts - your usage of this data must comply "
                   "with the terms of service of the account provider or application "
                   "from which the data was synced!");
        }
    }

    if (import) {
        // Read the contacts from the VCF
        QVersitReader reader(&vcf);
        reader.startReading();
        reader.waitForFinished();

        // Get the import list which duplicates coalesced, and updates merged
        int newCount = reader.results().count();
        int updatedCount = 0;
        QList<QContact> importedContacts(SeasideImport::buildImportContacts(reader.results(), &newCount, &updatedCount, nullptr, nullptr, !collectionIsLocalAddressbook));
        for (int i = 0; i < importedContacts.size(); ++i) {
            QContact &c(importedContacts[i]);
            c.setCollectionId(collectionId);
        }

        QString existingDesc(updatedCount ? QString::fromLatin1(" (updating %1 existing)").arg(updatedCount) : QString());
        qDebug("Importing %d new contacts %s", newCount, qPrintable(existingDesc));

        int importedCount = 0;

        while (!importedContacts.isEmpty()) {
            QMap<int, QContactManager::Error> errors;
            mgr.saveContacts(&importedContacts, &errors);
            importedCount += (importedContacts.count() - errors.count());

            QList<QContact> retryContacts;
            QMap<int, QContactManager::Error>::const_iterator eit = errors.constBegin(), eend = errors.constEnd();
            for ( ; eit != eend; ++eit) {
                const QContact &failed(importedContacts.at(eit.key()));
                if (eit.value() == QContactManager::LockedError) {
                    // This contact was part of a failed batch - we should retry
                    retryContacts.append(failed);
                } else {
                    qDebug() << "  Unable to import contact" << failed.detail<QContactDisplayLabel>().label() << "error:" << eit.value();
                }
            }

            importedContacts = retryContacts;
        }
        qDebug("Wrote %d contacts", importedCount);
    } else {
        QList<QContact> contacts(mgr.contacts(collectionFilter(collectionId)));

        QList<QVersitDocument> documents(SeasideExport::buildExportContacts(contacts));
        qDebug("Exporting %d contacts", documents.count());

        QVersitWriter writer(&vcf);
        writer.startWriting(documents);
        writer.waitForFinished();
        qDebug("Wrote %d contacts", documents.count());
    }

    return 0;
}

