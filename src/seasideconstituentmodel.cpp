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

#include "seasideconstituentmodel.h"
#include "seasideperson.h"

#include <QQmlInfo>

#include <QDebug>

/*!
  \qmltype ConstituentModel
  \inqmlmodule org.nemomobile.contacts
*/
SeasideConstituentModel::SeasideConstituentModel(QObject *parent)
    : SeasideSimpleContactModel(parent)
{
}

SeasideConstituentModel::~SeasideConstituentModel()
{
}

/*!
  \qmlproperty Person ConstituentModel::person
*/
SeasidePerson* SeasideConstituentModel::person() const
{
    return m_person;
}

void SeasideConstituentModel::setPerson(SeasidePerson *person)
{
    if (m_person != person) {
        if (m_person) {
            m_person->disconnect(this);
            m_person = nullptr;
        }
        if (m_cacheItem) {
            m_cacheItem = nullptr;
        }

        m_person = person;
        reset();
        emit personChanged();
    }
}

void SeasideConstituentModel::reset()
{
    if (!m_complete) {
        return;
    }

    if (m_person) {
        connect(m_person, &SeasidePerson::constituentsChanged,
                this, &SeasideConstituentModel::personConstituentsChanged);

        SeasideCache::CacheItem *cacheItem = SeasideCache::itemById(m_person->id());
        if (cacheItem) {
            m_cacheItem = cacheItem;
            m_person->fetchConstituents();
        } else {
            qmlInfo(this) << "Cannot find cache item for contact:" << m_person->id();
        }

    } else if (m_contacts.count() > 0) {
        setContactIds(QList<int>());
    }
}

void SeasideConstituentModel::personConstituentsChanged()
{
    if (m_person) {
        setContactIds(m_person->constituents());
    }
}

void SeasideConstituentModel::itemUpdated(SeasideCache::CacheItem *item)
{
    if (item == m_cacheItem) {
        m_person->fetchConstituents();
    }
    SeasideSimpleContactModel::itemUpdated(item);
}

void SeasideConstituentModel::itemAboutToBeRemoved(SeasideCache::CacheItem *item)
{
    if (item == m_cacheItem) {
        setPerson(nullptr);
    }
    SeasideSimpleContactModel::itemAboutToBeRemoved(item);
}
