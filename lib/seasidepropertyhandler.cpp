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
 *   * Neither the name of Nemo Mobile nor Jolla Ltd nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#include "seasidepropertyhandler.h"

#include <QContactAvatar>
#include <QContactOnlineAccount>
#include <QContactPresence>
#include <QContactSyncTarget>
#include <QCryptographicHash>
#include <QDir>
#include <QImage>

#include <qtcontacts-extensions.h>

namespace {

QContactAvatar avatarFromPhotoProperty(const QVersitProperty &property)
{
    // if the property is a PHOTO property, store the data to disk
    // and then create an avatar detail which points to it.

    // The data might be either a URL, a file path, or encoded image data
    // It's hard to tell what the content is, because versit removes the encoding
    // information in the process of decoding the data...

    // Try to interpret the data as a URL
    QString path(property.variantValue().toString());
    QUrl url(path);
    if (url.isValid()) {
        // Treat remote URL as a true URL, and reference it in the avatar
        if (!url.scheme().isEmpty() && !url.isLocalFile()) {
            QContactAvatar newAvatar;
            newAvatar.setImageUrl(url);

            // we have successfully processed this PHOTO property.
            return newAvatar;
        }
    }

    if (!url.isValid()) {
        // See if we can resolve the data as a local file path
        url = QUrl::fromLocalFile(path);
    }

    QByteArray photoData;

    if (url.isValid()) {
        // Try to read the data from the referenced file
        const QString filePath(url.path());
        if (QFile::exists(filePath)) {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Unable to process photo data as file:" << path;
                return QContactAvatar();
            } else {
                photoData = file.readAll();
            }
        }
    }

    if (photoData.isEmpty()) {
        // Try to interpret the encoded property data as the image
        photoData = property.variantValue().toByteArray();
        if (photoData.isEmpty()) {
            qWarning() << "Failed to extract avatar data from vCard PHOTO property";
            return QContactAvatar();
        }
    }

    QImage img;
    bool loaded = img.loadFromData(photoData);
    if (!loaded) {
        qWarning() << "Failed to load avatar image from vCard PHOTO data";
        return QContactAvatar();
    }

    // We will save the avatar image to disk in the system's data location
    // Since we're importing user data, it should not require privileged access
    const QString subdirectory(QString::fromLatin1(".local/share/system/Contacts/avatars"));
    const QString photoDirPath(QDir::home().filePath(subdirectory));

    // create the photo file dir if it doesn't exist.
    QDir photoDir;
    if (!photoDir.mkpath(photoDirPath)) {
        qWarning() << "Failed to create avatar image directory when loading avatar image from vCard PHOTO data";
        return QContactAvatar();
    }

    // construct the filename of the new avatar image.
    QString photoFilePath = QString::fromLatin1(QCryptographicHash::hash(photoData, QCryptographicHash::Md5).toHex());
    photoFilePath = photoDirPath + QDir::separator() + photoFilePath + QString::fromLatin1(".jpg");

    // save the file to disk
    bool saved = img.save(photoFilePath);
    if (!saved) {
        qWarning() << "Failed to save avatar image from vCard PHOTO data to" << photoFilePath;
        return QContactAvatar();
    }

    qWarning() << "Successfully saved avatar image from vCard PHOTO data to" << photoFilePath;

    // save the avatar detail - TODO: mark the avatar as "owned by the contact" (remove on delete)
    QContactAvatar newAvatar;
    newAvatar.setImageUrl(QUrl::fromLocalFile(photoFilePath));

    // we have successfully processed this PHOTO property.
    return newAvatar;
}

void processPhoto(const QVersitProperty &property, bool *alreadyProcessed, QList<QContactDetail> * updatedDetails)
{
    QContactAvatar newAvatar = avatarFromPhotoProperty(property);
    if (!newAvatar.isEmpty()) {
        updatedDetails->append(newAvatar);
        *alreadyProcessed = true;
    }
}

void processOnlineAccount(const QVersitProperty &property, bool *alreadyProcessed, QList<QContactDetail> * updatedDetails)
{
    // Create an online account instance for demo purposes; it will not be connected
    // to a registered telepathy account, so it won't actually be able to converse

    // Try to interpret the data as a stringlist
    const QString detail(property.variantValue().toString());

    // The format is: URI/path/display-name/icon-path/service-provider/service-provider-display-name
    const QStringList details(detail.split(QLatin1Char(';'), QString::KeepEmptyParts));
    if (details.count() == 6) {
        QContactOnlineAccount qcoa;

        qcoa.setValue(QContactOnlineAccount::FieldAccountUri, details.at(0));
        qcoa.setValue(QContactOnlineAccount__FieldAccountPath, details.at(1));
        qcoa.setValue(QContactOnlineAccount__FieldAccountDisplayName, details.at(2));
        qcoa.setValue(QContactOnlineAccount__FieldAccountIconPath, details.at(3));
        qcoa.setValue(QContactOnlineAccount::FieldServiceProvider, details.at(4));
        qcoa.setValue(QContactOnlineAccount__FieldServiceProviderDisplayName, details.at(5));
        qcoa.setDetailUri(QString::fromLatin1("%1:%2").arg(details.at(1)).arg(details.at(0)));

        updatedDetails->append(qcoa);

        // Since it is a demo account, give it a random presence state
        const int state = (qrand() % 4);
        QContactPresence presence;
        presence.setPresenceState(state == 3 ? QContactPresence::PresenceBusy : (state == 2 ? QContactPresence::PresenceAway : QContactPresence::PresenceAvailable));
        presence.setLinkedDetailUris(QStringList() << qcoa.detailUri());
        updatedDetails->append(presence);

        *alreadyProcessed = true;
    } else {
        qWarning() << "Invalid online account details:" << details;
    }
}

void processSyncTarget(const QVersitProperty &property, bool *alreadyProcessed, QList<QContactDetail> * updatedDetails)
{
    // Set the sync target for this contact, if appropriate

    // Try to interpret the data as a string
    const QString data(property.variantValue().toString().toLower());

    if (data == QString::fromLatin1("bluetooth") || data == QString::fromLatin1("was_local")) {
        QList<QContactDetail>::iterator it = updatedDetails->begin(), end = updatedDetails->end();
        for ( ; it != end; ++it) {
            if ((*it).type() == QContactSyncTarget::Type)
                break;
        }
        if (it != end) {
            QContactSyncTarget &syncTarget(static_cast<QContactSyncTarget &>(*it));
            syncTarget.setSyncTarget(data);
        } else {
            QContactSyncTarget syncTarget;
            syncTarget.setSyncTarget(data);
            updatedDetails->append(syncTarget);
        }

        *alreadyProcessed = true;
    } else {
        qWarning() << "Invalid syncTarget data:" << data;
    }
}

void processSyncTarget(const QContactSyncTarget &detail, QSet<int> * processedFields, QList<QVersitProperty> * toBeRemoved, QList<QVersitProperty> * toBeAdded)
{
    Q_UNUSED(processedFields)
    Q_UNUSED(toBeRemoved)

    // Include the sync target in the export if it is not 'local' but is exportable
    const QString syncTarget(detail.syncTarget());

    if (syncTarget == QString::fromLatin1("bluetooth") || syncTarget == QString::fromLatin1("was_local")) {
        QVersitProperty stProperty;
        stProperty.setName(QString::fromLatin1("X-NEMOMOBILE-SYNCTARGET"));
        stProperty.setValue(syncTarget);

        toBeAdded->append(stProperty);
    } else if (!syncTarget.isEmpty() && syncTarget != QString::fromLatin1("local")) {
        qWarning() << "Invalid syncTarget for export:" << syncTarget;
    }
}

void ignoreDetail(const QContactSyncTarget &detail, QSet<int> * processedFields, QList<QVersitProperty> * toBeRemoved, QList<QVersitProperty> * toBeAdded)
{
    Q_UNUSED(detail)
    Q_UNUSED(processedFields)
    toBeAdded->clear();
    toBeRemoved->clear();
}

}

class SeasidePropertyHandlerPrivate
{
public:
    QSet<QContactDetail::DetailType> m_nonexportableDetails;
};

SeasidePropertyHandler::SeasidePropertyHandler(const QSet<QContactDetail::DetailType> &nonexportableDetails)
    : QVersitContactHandler()
    , priv(new SeasidePropertyHandlerPrivate)
{
    priv->m_nonexportableDetails = nonexportableDetails;
}

SeasidePropertyHandler::~SeasidePropertyHandler()
{
    delete priv;
}

void SeasidePropertyHandler::documentProcessed(const QVersitDocument &, QContact *)
{
    // do nothing, have no state to clean.
}

void SeasidePropertyHandler::propertyProcessed(const QVersitDocument &, const QVersitProperty &property,
                                               const QContact &, bool *alreadyProcessed, QList<QContactDetail> * updatedDetails)
{
    const QString propertyName(property.name().toLower());

    if (propertyName == QLatin1String("photo")) {
        processPhoto(property, alreadyProcessed, updatedDetails);
    } else if (propertyName == QLatin1String("x-nemomobile-onlineaccount-demo")) {
        processOnlineAccount(property, alreadyProcessed, updatedDetails);
    } else if (propertyName == QLatin1String("x-nemomobile-synctarget")) {
        processSyncTarget(property, alreadyProcessed, updatedDetails);
    }
}

void SeasidePropertyHandler::contactProcessed(const QContact &, QVersitDocument *)
{
}

void SeasidePropertyHandler::detailProcessed(const QContact &, const QContactDetail &detail,
                                             const QVersitDocument &, QSet<int> * processedFields, QList<QVersitProperty> * toBeRemoved, QList<QVersitProperty> * toBeAdded)
{
    const QContactDetail::DetailType detailType(detail.type());

    if (priv->m_nonexportableDetails.contains(detailType)) {
        ignoreDetail(static_cast<const QContactSyncTarget &>(detail), processedFields, toBeRemoved, toBeAdded);
    } else if (detailType == QContactSyncTarget::Type) {
        processSyncTarget(static_cast<const QContactSyncTarget &>(detail), processedFields, toBeRemoved, toBeAdded);
    }
}

QContactAvatar SeasidePropertyHandler::avatarFromPhotoProperty(const QVersitProperty &property)
{
    return ::avatarFromPhotoProperty(property);
}
