/*
 * Copyright (c) 2014 - 2020 Jolla Ltd.
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

#include "cacheconfiguration.h"

#include <QtDebug>

CacheConfiguration::CacheConfiguration()
    : m_displayLabelOrder(FirstNameFirst)
    , m_sortProperty(QString::fromLatin1("firstName"))
    , m_groupProperty(QString::fromLatin1("firstName"))
#ifdef HAS_MLITE
    , m_displayLabelOrderConf(QLatin1String("/org/nemomobile/contacts/display_label_order"))
    , m_sortPropertyConf(QLatin1String("/org/nemomobile/contacts/sort_property"))
    , m_groupPropertyConf(QLatin1String("/org/nemomobile/contacts/group_property"))
#endif
{
#ifdef HAS_MLITE
    connect(&m_displayLabelOrderConf, SIGNAL(valueChanged()), this, SLOT(onDisplayLabelOrderChanged()));
    QVariant displayLabelOrder = m_displayLabelOrderConf.value();
    if (displayLabelOrder.isValid())
        m_displayLabelOrder = static_cast<DisplayLabelOrder>(displayLabelOrder.toInt());

    connect(&m_sortPropertyConf, SIGNAL(valueChanged()), this, SLOT(onSortPropertyChanged()));
    QVariant sortPropertyConf = m_sortPropertyConf.value();
    if (sortPropertyConf.isValid())
        m_sortProperty = sortPropertyConf.toString();

    connect(&m_groupPropertyConf, SIGNAL(valueChanged()), this, SLOT(onGroupPropertyChanged()));
    QVariant groupPropertyConf = m_groupPropertyConf.value();
    if (groupPropertyConf.isValid())
        m_groupProperty = groupPropertyConf.toString();
#endif
}

#ifdef HAS_MLITE
void CacheConfiguration::onDisplayLabelOrderChanged()
{
    QVariant displayLabelOrder = m_displayLabelOrderConf.value();
    if (displayLabelOrder.isValid() && displayLabelOrder.toInt() != m_displayLabelOrder) {
        m_displayLabelOrder = static_cast<DisplayLabelOrder>(displayLabelOrder.toInt());
        emit displayLabelOrderChanged(m_displayLabelOrder);
    }
}

void CacheConfiguration::onSortPropertyChanged()
{
    QVariant sortProperty = m_sortPropertyConf.value();
    if (sortProperty.isValid() && sortProperty.toString() != m_sortProperty) {
        const QString newProperty(sortProperty.toString());
        if ((newProperty != QString::fromLatin1("firstName")) &&
            (newProperty != QString::fromLatin1("lastName"))) {
            qWarning() << "Invalid sort property configuration:" << newProperty;
            return;
        }

        m_sortProperty = newProperty;
        emit sortPropertyChanged(m_sortProperty);
    }
}

void CacheConfiguration::onGroupPropertyChanged()
{
    QVariant groupProperty = m_groupPropertyConf.value();
    if (groupProperty.isValid() && groupProperty.toString() != m_groupProperty) {
        const QString newProperty(groupProperty.toString());
        if ((newProperty != QString::fromLatin1("firstName")) &&
            (newProperty != QString::fromLatin1("lastName"))) {
            qWarning() << "Invalid group property configuration:" << newProperty;
            return;
        }

        m_groupProperty = newProperty;
        emit groupPropertyChanged(m_groupProperty);
    }
}
#endif

