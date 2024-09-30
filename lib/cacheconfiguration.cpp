/*
 * Copyright (c) 2014 - 2020 Jolla Ltd.
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

#include "cacheconfiguration.h"

#include <QtDebug>

#ifdef HAS_MLITE
#include <mgconfitem.h>
#endif

#ifdef HAS_QGSETTINGS
#include <QGSettings>
#endif

class CacheConfigurationPrivate : public QObject
{
    Q_OBJECT
public:
    CacheConfigurationPrivate(CacheConfiguration *q);

    CacheConfiguration *q_ptr;
    CacheConfiguration::DisplayLabelOrder m_displayLabelOrder;
    QString m_sortProperty;
    QString m_groupProperty;

#ifdef HAS_MLITE
    MGConfItem m_displayLabelOrderConf;
    MGConfItem m_sortPropertyConf;
    MGConfItem m_groupPropertyConf;

    void onDisplayLabelOrderChanged();
    void onSortPropertyChanged();
    void onGroupPropertyChanged();
#endif

#ifdef HAS_QGSETTINGS
    QGSettings *m_propertyConf;

    void onConfigPropertyChanged(const QString &key);
#endif // HAS_QGSETTINGS
};

CacheConfigurationPrivate::CacheConfigurationPrivate(CacheConfiguration *q)
    : q_ptr(q)
    , m_displayLabelOrder(CacheConfiguration::FirstNameFirst)
    , m_sortProperty(QString::fromLatin1("firstName"))
    , m_groupProperty(QString::fromLatin1("firstName"))
#ifdef HAS_MLITE
    , m_displayLabelOrderConf(QLatin1String("/org/nemomobile/contacts/display_label_order"))
    , m_sortPropertyConf(QLatin1String("/org/nemomobile/contacts/sort_property"))
    , m_groupPropertyConf(QLatin1String("/org/nemomobile/contacts/group_property"))
#endif
#ifdef HAS_QGSETTINGS
    , m_propertyConf(new QGSettings("org.nemomobile.contacts", "/org/nemomobile/contacts/"))
#endif // HAS_QGSETTINGS

{
#ifdef HAS_MLITE
    connect(&m_displayLabelOrderConf, &MGConfItem::valueChanged,
            this, &CacheConfigurationPrivate::onDisplayLabelOrderChanged);
    QVariant displayLabelOrder = m_displayLabelOrderConf.value();
    if (displayLabelOrder.isValid())
        m_displayLabelOrder = static_cast<CacheConfiguration::DisplayLabelOrder>(displayLabelOrder.toInt());

    connect(&m_sortPropertyConf, &MGConfItem::valueChanged,
            this, &CacheConfigurationPrivate::onSortPropertyChanged);
    QVariant sortPropertyConf = m_sortPropertyConf.value();
    if (sortPropertyConf.isValid())
        m_sortProperty = sortPropertyConf.toString();

    connect(&m_groupPropertyConf, &MGConfItem::valueChanged,
            this, &CacheConfigurationPrivate::onGroupPropertyChanged);
    QVariant groupPropertyConf = m_groupPropertyConf.value();
    if (groupPropertyConf.isValid())
        m_groupProperty = groupPropertyConf.toString();
#endif

#ifdef HAS_QGSETTINGS
    connect(m_propertyConf, &QGSettings::changed, this,
            &CacheConfigurationPrivate::onConfigPropertyChanged);

    QVariant displayLabelOrder = m_propertyConf->get(QStringLiteral("display-label-order"));
    if (displayLabelOrder.isValid())
        m_displayLabelOrder = static_cast<CacheConfiguration::DisplayLabelOrder>(displayLabelOrder.toInt());

    QVariant sortPropertyConf = m_propertyConf->get(QStringLiteral("sort-property"));
    if (sortPropertyConf.isValid())
        m_sortProperty = sortPropertyConf.toString();

    QVariant groupPropertyConf = m_propertyConf->get(QStringLiteral("group-property"));
    if (groupPropertyConf.isValid())
        m_groupProperty = groupPropertyConf.toString();

#endif
}

#ifdef HAS_MLITE
void CacheConfigurationPrivate::onDisplayLabelOrderChanged()
{
    QVariant displayLabelOrder = m_displayLabelOrderConf.value();
    if (displayLabelOrder.isValid() && displayLabelOrder.toInt() != m_displayLabelOrder) {
        m_displayLabelOrder = static_cast<CacheConfiguration::DisplayLabelOrder>(displayLabelOrder.toInt());
        emit q_ptr->displayLabelOrderChanged(m_displayLabelOrder);
    }
}

void CacheConfigurationPrivate::onSortPropertyChanged()
{
    QVariant sortProperty = m_sortPropertyConf.value();
    if (sortProperty.isValid() && sortProperty.toString() != m_sortProperty) {
        const QString newProperty(sortProperty.toString());
        if ((newProperty != QString::fromLatin1("firstName"))
                && (newProperty != QString::fromLatin1("lastName"))) {
            qWarning() << "Invalid sort property configuration:" << newProperty;
            return;
        }

        m_sortProperty = newProperty;
        emit q_ptr->sortPropertyChanged(m_sortProperty);
    }
}

void CacheConfigurationPrivate::onGroupPropertyChanged()
{
    QVariant groupProperty = m_groupPropertyConf.value();
    if (groupProperty.isValid() && groupProperty.toString() != m_groupProperty) {
        const QString newProperty(groupProperty.toString());
        if ((newProperty != QString::fromLatin1("firstName"))
                && (newProperty != QString::fromLatin1("lastName"))) {
            qWarning() << "Invalid group property configuration:" << newProperty;
            return;
        }

        m_groupProperty = newProperty;
        emit q_ptr->groupPropertyChanged(m_groupProperty);
    }
}
#endif

#ifdef HAS_QGSETTINGS
void CacheConfigurationPrivate::onConfigPropertyChanged(const QString &key) {
    if (key == QLatin1String("display-label-order")) {
        QVariant displayLabelOrder = m_propertyConf->get(QStringLiteral("display-label-order"));
        if (displayLabelOrder.isValid() && displayLabelOrder.toInt() != m_displayLabelOrder) {
            m_displayLabelOrder = static_cast<CacheConfiguration::DisplayLabelOrder>(displayLabelOrder.toInt());
            emit q_ptr->displayLabelOrderChanged(m_displayLabelOrder);
        }
    } else if (key == QLatin1String("sort-property")) {
        QVariant sortProperty = m_propertyConf->get(QStringLiteral("sort-property"));
        if (sortProperty.isValid() && sortProperty.toString() != m_sortProperty) {
            const QString newProperty(sortProperty.toString());
            if ((newProperty != QString::fromLatin1("firstName")) &&
                (newProperty != QString::fromLatin1("lastName"))) {
                qWarning() << "Invalid sort property configuration:" << newProperty;
                return;
            }

            m_sortProperty = newProperty;
            emit q_ptr->sortPropertyChanged(m_sortProperty);
        }
    } else if (key == QLatin1String("group-property")) {

        QVariant groupProperty = m_propertyConf->get(QStringLiteral("group-property"));
        if (groupProperty.isValid() && groupProperty.toString() != m_groupProperty) {
            const QString newProperty(groupProperty.toString());
            if ((newProperty != QString::fromLatin1("firstName")) &&
                (newProperty != QString::fromLatin1("lastName"))) {
                qWarning() << "Invalid group property configuration:" << newProperty;
                return;
            }

            m_groupProperty = newProperty;
            emit q_ptr->groupPropertyChanged(m_groupProperty);
        }
    }
}
#endif

CacheConfiguration::CacheConfiguration()
    : d_ptr(new CacheConfigurationPrivate(this))
{
}

CacheConfiguration::~CacheConfiguration()
{
    delete d_ptr;
}

CacheConfiguration::DisplayLabelOrder CacheConfiguration::displayLabelOrder() const
{
    return d_ptr->m_displayLabelOrder;
}

QString CacheConfiguration::sortProperty() const
{
    return d_ptr->m_sortProperty;
}

QString CacheConfiguration::groupProperty() const
{
    return d_ptr->m_groupProperty;
}

#include "cacheconfiguration.moc"
