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

#ifndef SEASIDEADDRESSBOOK_H
#define SEASIDEADDRESSBOOK_H

// Qt
#include <QObject>
#include <QColor>

// Mobility
#include <QContactCollection>

QTCONTACTS_USE_NAMESPACE

class SeasideAddressBook
{
    Q_GADGET
    Q_PROPERTY(QString id READ idString CONSTANT)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QColor color MEMBER color)
    Q_PROPERTY(QColor secondaryColor MEMBER secondaryColor)
    Q_PROPERTY(QString image MEMBER image CONSTANT)
    Q_PROPERTY(bool isAggregate MEMBER isAggregate)
    Q_PROPERTY(bool isLocal MEMBER isLocal)

public:
    SeasideAddressBook();
    ~SeasideAddressBook();

    bool operator==(const SeasideAddressBook &other);
    inline bool operator!=(const SeasideAddressBook &other) { return !(operator==(other)); }

    QString idString() const;
    QString displayName() const;

    static SeasideAddressBook fromCollectionId(const QContactCollectionId &id);

    QContactCollectionId collectionId;
    QString name;
    QColor color;
    QColor secondaryColor;
    QString image;
    bool isAggregate = false;
    bool isLocal = false;
};

Q_DECLARE_METATYPE(SeasideAddressBook)

#endif // SEASIDEADDRESSBOOK_H

