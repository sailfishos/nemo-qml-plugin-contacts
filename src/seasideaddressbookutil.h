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

#ifndef SEASIDEADDRESSBOOKUTIL_H
#define SEASIDEADDRESSBOOKUTIL_H

#include "seasideaddressbook.h"

#include <QVariantList>

QTCONTACTS_USE_NAMESPACE

class SeasideAddressBookUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList addressBooks READ addressBooks NOTIFY addressBooksChanged)

public:
    SeasideAddressBookUtil(QObject *parent = 0);
    ~SeasideAddressBookUtil();

    QVariantList addressBooks() const;

signals:
    void addressBooksChanged();

private:
    void collectionsAdded(const QList<QContactCollectionId> &collectionIds);
    void collectionsRemoved(const QList<QContactCollectionId> &collectionIds);
    void collectionsChanged(const QList<QContactCollectionId> &collectionIds);
    int findCollection(const QContactCollectionId &id) const;

    QVariantList m_addressBooks;
 };

#endif
