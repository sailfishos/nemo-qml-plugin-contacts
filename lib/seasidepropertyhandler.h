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

#ifndef PROPERTYHANDLER_H
#define PROPERTYHANDLER_H

#include "contactcacheexport.h"

#include <QString>
#include <QByteArray>
#include <QVariantMap>

#include <QContact>

#include <QVersitContactHandler>
#include <QVersitResourceHandler>
#include <QVersitDocument>
#include <QVersitProperty>

#include <QContactAvatar>

QTCONTACTS_USE_NAMESPACE
QTVERSIT_USE_NAMESPACE

/*
    SeasidePropertyHandler

    Some backends don't support saving PHOTO data directly.
    Instead, the PHOTO data needs to be extracted, saved to
    a file, and then the path to the file needs to be saved
    to the backend as a contact avatar url detail.

    The X-NEMOMOBILE-SYNCTARGET property is supported for specifying
    the sync target of a contact.

    Also support the X-NEMOMOBILE-ONLINEACCOUNT-DEMO property
    for loading demo online account data.
*/
class SeasidePropertyHandlerPrivate;
class CONTACTCACHE_EXPORT SeasidePropertyHandler : public QVersitContactHandler
{
public:
    SeasidePropertyHandler(const QSet<QContactDetail::DetailType> &nonexportableDetails = QSet<QContactDetail::DetailType>());
    ~SeasidePropertyHandler();

    // QVersitContactImporterPropertyHandlerV2
    void documentProcessed(const QVersitDocument &, QContact *);
    void propertyProcessed(const QVersitDocument &, const QVersitProperty &property,
                           const QContact &, bool *alreadyProcessed, QList<QContactDetail> * updatedDetails);

    // QVersitContactExporterDetailHandlerV2
    void contactProcessed(const QContact &, QVersitDocument *);
    void detailProcessed(const QContact &, const QContactDetail &detail,
                         const QVersitDocument &, QSet<int> * processedFields, QList<QVersitProperty> * toBeRemoved, QList<QVersitProperty> * toBeAdded);

    static QContactAvatar avatarFromPhotoProperty(const QVersitProperty &property);

private:
    SeasidePropertyHandlerPrivate *priv;
};

#endif // PROPERTYHANDLER_H
