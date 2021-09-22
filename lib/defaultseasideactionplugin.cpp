/*
 * Copyright (c) 2021 Jolla Ltd.
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

#include "defaultseasideactionplugin.h"
#include "seasidecache.h"

#include <QContactAddress>
#include <QContactPhoneNumber>
#include <QContactEmailAddress>
#include <QContactAnniversary>
#include <QContactBirthday>
#include <QContactUrl>
#include <QContactAction>

#include <QFile>
#include <QDesktopServices>
#include <QDate>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDebug>

namespace {

bool sendDBusMessage(const QDBusMessage &message, const QDBusConnection &conn)
{
    const QDBusMessage result = conn.call(message, QDBus::NoBlock);
    if (result.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "D-Bus call to" << result.service() << result.path() << result.interface()
                   << result.member() << "failed:" << result.errorName() << result.errorMessage();
        return false;
    }
    return true;
}

bool callPhoneNumber(const QString &number, const QString &modemPath)
{
    QString methodName;
    QVariantList args;
    if (modemPath.isEmpty()) {
        methodName = QStringLiteral("dial");
        args << number;
    } else {
        methodName = QStringLiteral("dialViaModem");
        args << modemPath << number;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
                QStringLiteral("com.jolla.voicecall.ui"),
                QStringLiteral("/"),
                QStringLiteral("com.jolla.voicecall.ui"),
                methodName);
    message.setArguments(args);
    return sendDBusMessage(message, QDBusConnection::sessionBus());
}

QDBusMessage messagesAppDBusMessage(const QString &methodName)
{
    return QDBusMessage::createMethodCall(
                    QStringLiteral("org.sailfishos.Messages"),
                    QStringLiteral("/"),
                    QStringLiteral("org.sailfishos.Messages"),
                    methodName);
}

bool viewCalendarDate(const QDate &date)
{
    if (!QFile::exists(QLatin1String("/usr/share/applications/jolla-calendar.desktop"))) {
        return false;
    }

    // Rather than showing the contact's actual birth or anniversary date in the calendar,
    // show the next occurrence of the anniversary of the recorded date
    const QDate now = QDate::currentDate();
    QDate nextDate(now.year(), date.month(), date.day());
    if (now > nextDate) {
        nextDate = nextDate.addYears(1);
    }
    const QString formattedDate = nextDate.toString(Qt::ISODate);

    QDBusMessage message = QDBusMessage::createMethodCall(
                        QStringLiteral("com.jolla.calendar.ui"),
                        QStringLiteral("/com/jolla/calendar/ui"),
                        QStringLiteral("com.jolla.calendar.ui"),
                        QStringLiteral("viewDate"));
    message.setArguments(QVariantList() << formattedDate);
    return sendDBusMessage(message, QDBusConnection::sessionBus());
}

bool viewAddress(const QContactAddress &address)
{
    if (!QFile::exists(QLatin1String("/usr/share/applications/sailfish-maps.desktop"))) {
        return false;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
                        QStringLiteral("org.sailfishos.maps"),
                        QStringLiteral("/"),
                        QStringLiteral("org.sailfishos.maps"),
                        QStringLiteral("openAddress"));
    message.setArguments(QVariantList()
                         << address.street()
                         << address.locality()
                         << address.region()
                         << address.postcode()
                         << address.country());
    return sendDBusMessage(message, QDBusConnection::sessionBus());
}

}

QList<SeasideActionPlugin::ActionInfo> DefaultSeasideActionPlugin::supportedActions(
        const QContact &contact, const QContactDetail::DetailType &detailType)
{
    Q_UNUSED(contact)

    QList<ActionInfo> infoList;

    switch (detailType) {
    case QContactDetail::TypeAddress:
        infoList << ActionInfo{ QContactAction::ActionOpenInViewer(), QStringLiteral("image://theme/icon-m-location") };
        break;
    case QContactDetail::TypeAnniversary:
        infoList << ActionInfo{ QContactAction::ActionOpenInViewer(), QStringLiteral("image://theme/icon-m-date") };
        break;
    case QContactDetail::TypeBirthday:
        infoList << ActionInfo{ QContactAction::ActionOpenInViewer(), QStringLiteral("image://theme/icon-m-date") };
        break;
    case QContactDetail::TypeEmailAddress:
        infoList << ActionInfo{ QContactAction::ActionOpenInViewer(), QStringLiteral("image://theme/icon-m-mail") };
        break;
    case QContactDetail::TypeOnlineAccount:
        infoList << ActionInfo{ QContactAction::ActionChat(), QStringLiteral("image://theme/icon-m-message") };
        break;
    case QContactDetail::TypePhoneNumber:
        infoList << ActionInfo{ QContactAction::ActionCall(), QStringLiteral("image://theme/icon-m-call") };
        infoList << ActionInfo{ QContactAction::ActionSms(), QStringLiteral("image://theme/icon-m-message") };
        break;
    case QContactDetail::TypeNote:
        infoList << ActionInfo{ QContactAction::ActionOpenInViewer(), QStringLiteral("image://theme/icon-m-note") };
        break;
    case QContactDetail::TypeUrl:
        infoList << ActionInfo{ QContactAction::ActionOpenInViewer(), QStringLiteral("image://theme/icon-m-website") };
        break;
    default:
        qWarning() << "supportedActions(): Unhandled detail type" << detailType;
        break;
    }

    return infoList;
}

bool DefaultSeasideActionPlugin::triggerAction(const QString &actionType,
        const QVariantMap &parameters, const QContact &contact, const QContactDetail &detail)
{
    Q_UNUSED(contact)

    switch (detail.type()) {
    case QContactDetail::TypeAddress:
    {
        return viewAddress(static_cast<QContactAddress>(detail));
    }
    case QContactDetail::TypeAnniversary:
    {
        return viewCalendarDate(detail.value(QContactAnniversary::FieldOriginalDate).toDate());
    }
    case QContactDetail::TypeBirthday:
    {
        return viewCalendarDate(detail.value(QContactBirthday::FieldBirthday).toDate());
    }
    case QContactDetail::TypeEmailAddress:
    {
        const QString email = detail.value(QContactEmailAddress::FieldEmailAddress).toString();
        const QUrl url(QStringLiteral("mailto:") + email);
        return QDesktopServices::openUrl(url);
    }
    case QContactDetail::TypeOnlineAccount:
    {
        const QString localUid = detail.value(QContactOnlineAccount__FieldAccountPath).toString();
        const QString remoteUid = detail.value(QContactOnlineAccount::FieldAccountUri).toString();
        QDBusMessage message = messagesAppDBusMessage(QStringLiteral("startConversation"));
        message.setArguments(QVariantList() << localUid << remoteUid);
        return sendDBusMessage(message, QDBusConnection::sessionBus());
    }
    case QContactDetail::TypePhoneNumber:
    {
        const QString number = SeasideCache::normalizePhoneNumber(detail.value(QContactPhoneNumber::FieldNumber).toString());
        if (actionType == QContactAction::ActionCall()) {
            return callPhoneNumber(number, parameters.value(QStringLiteral("modemPath")).toString());
        } else if (actionType == QContactAction::ActionSms()) {
            QDBusMessage message = messagesAppDBusMessage(QStringLiteral("startSMS"));
            message.setArguments(QVariantList() << number);
            return sendDBusMessage(message, QDBusConnection::sessionBus());
        }
        break;
    }
    case QContactDetail::TypeUrl:
    {
        return QDesktopServices::openUrl(detail.value(QContactUrl::FieldUrl).toString());
    }
    default:
        break;
    }

    qWarning() << "Unsupported action" << actionType << "detail type" << detail.type();
    return false;
}
