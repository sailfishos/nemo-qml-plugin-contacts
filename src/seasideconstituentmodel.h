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

#ifndef SEASIDECONSTITUENTMODEL_H
#define SEASIDECONSTITUENTMODEL_H

#include "seasidesimplecontactmodel.h"

class SeasidePerson;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#ifndef DECLARE_SEASIDE_PERSON
#define DECLARE_SEASIDE_PERSON
Q_DECLARE_OPAQUE_POINTER(SeasidePerson)
#endif
#endif
QTCONTACTS_USE_NAMESPACE

class SeasideConstituentModel : public SeasideSimpleContactModel
{
    Q_OBJECT
    Q_PROPERTY(SeasidePerson* person READ person WRITE setPerson NOTIFY personChanged)

public:
    SeasideConstituentModel(QObject *parent = 0);
    ~SeasideConstituentModel();

    SeasidePerson* person() const;
    void setPerson(SeasidePerson *person);

    virtual void itemUpdated(SeasideCache::CacheItem *item) override;
    virtual void itemAboutToBeRemoved(SeasideCache::CacheItem *item) override;

Q_SIGNALS:
    void personChanged();

protected:
    virtual void reset() override;

private:
    void personConstituentsChanged();

    SeasidePerson *m_person = nullptr;
    SeasideCache::CacheItem *m_cacheItem = nullptr;
};
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
Q_MOC_INCLUDE("seasideperson.h")
#endif
#endif
