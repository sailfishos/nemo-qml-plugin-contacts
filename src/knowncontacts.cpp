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

#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <random>

#include "knowncontacts.h"

static const auto KnownContactsSyncFolder = QStringLiteral("system/privileged/Contacts/knowncontacts");
static const auto KnownContactsSyncProfile = QStringLiteral("knowncontacts.Contacts");
static const auto MsyncdService = QStringLiteral("com.meego.msyncd");
static const auto SynchronizerPath = QStringLiteral("/synchronizer");
static const auto MsyncdInterface = MsyncdService;
static const auto GalIdKey = QStringLiteral("id");
static const auto AccountIdKey = QStringLiteral("accountId");

/*!
  \qmltype KnownContacts
  \inqmlmodule org.nemomobile.contacts
*/
KnownContacts::KnownContacts(QObject *parent)
    : QObject(parent)
    , m_msyncd(MsyncdService, SynchronizerPath, MsyncdInterface)
{
    if (!m_msyncd.isValid())
        qWarning() << "Could not connect to msyncd: contacts are not synchronized automatically";
}

KnownContacts::~KnownContacts()
{
}

/*!
  \qmlmethod bool KnownContacts::storeContact(object contact)
*/
bool KnownContacts::storeContact(const QVariantMap &contact)
{
    return storeContacts({contact});
}

/*!
  \qmlmethod bool KnownContacts::storeContacts(array contacts)
*/
bool KnownContacts::storeContacts(const QVariantList &contacts)
{
    QMap<int, QList<QVariantMap> > accountContacts;

    for (const auto variant : contacts) {
        const QVariantMap contact = variant.toMap();
        if (contact.isEmpty()) {
            qWarning() << "Cannot store contacts: a contact is not a mapping";
            continue;
        }
        const QString contactGalId = contact.value(GalIdKey).toString();
        if (contactGalId.isEmpty()) {
            qWarning() << "Cannot store contact: missing value for key 'id'";
            continue;
        }
        const int accountId = contact.value(AccountIdKey).toInt();
        if (accountId <= 0) {
            qWarning() << "Cannot store contact: missing value for key 'accountId'";
            continue;
        }
        accountContacts[accountId].append(contact);
    }

    for (auto it = accountContacts.constBegin(); it != accountContacts.constEnd(); ++it) {
        const int accountId = it.key();

        QSettings file(getPath(accountId), QSettings::IniFormat, this);
        if (!file.isWritable()) {
            qWarning() << "Can not store contacts:" << file.fileName() << "is not writable";
            continue;
        }

        const QList<QVariantMap> &contacts = it.value();
        for (const QVariantMap &contact : contacts) {
            const QString contactGalId = contact.value(GalIdKey).toString();
            file.beginGroup(contactGalId);
            QMapIterator<QString, QVariant> iter(contact);
            while (iter.hasNext()) {
                iter.next();
                if (iter.key() != GalIdKey) {
                    file.setValue(iter.key(), iter.value());
                }
            }
            file.endGroup();
        }
        file.sync();
    }

    return synchronize();
}

quint32 KnownContacts::getRandomNumber()
{
    static std::random_device random;
    static std::mt19937 generator(random());
    static std::uniform_int_distribution<quint32> distribution;
    return distribution(generator);
}

QString KnownContacts::getRandomPath(int accountId)
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                QDir::separator() + KnownContactsSyncFolder + QDir::separator() +
                QStringLiteral("%1-contacts-%2.ini").arg(accountId).arg(getRandomNumber());
}

const QString & KnownContacts::getPath(int accountId)
{
    if (m_currentPath.isEmpty()) {
        auto path = getRandomPath(accountId);
        while (QFile::exists(path))
            path = getRandomPath(accountId);
        m_currentPath.swap(path);
    }
    return m_currentPath;
}

bool KnownContacts::synchronize()
{
    if (m_msyncd.isValid()) {
        QDBusPendingCall call = m_msyncd.asyncCall(QStringLiteral("startSync"), KnownContactsSyncProfile);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, &QDBusPendingCallWatcher::finished,
                this, &KnownContacts::syncStarted);
        return true;
    }
    qWarning() << "Can not start synchronizing knowncontacts: can not connect to msyncd";
    return false;
}

void KnownContacts::syncStarted(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<bool> reply = *call;
    if (reply.isValid()) {
        if (reply.value()) {
        } else {
            qWarning() << "Starting knowncontacts sync failed";
        }
    } else {
        qWarning() << "Starting knowncontacts sync failed:" << reply.error();
    }
    call->deleteLater();
}
