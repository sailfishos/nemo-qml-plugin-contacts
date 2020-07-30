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

#include "seasideimport.h"

#include <QContactGuid>
#include <QContactName>
#include <QContactNickname>
#include <QContactPhoneNumber>

#include <QVersitReader>

#include <QObject>
#include <QtTest>

QTVERSIT_USE_NAMESPACE

class tst_SeasideImport : public QObject
{
    Q_OBJECT

public:
    tst_SeasideImport();

    static QList<QContact> processVCard(const char *vCardData);

private slots:
    void name();
    void formattedName();
    void nickname();
    void unidentified();

    void nonmergedName();
    void nonmergedNickname();
    void nonmergedUid();

    void mergedName();
    void mergedNickname();
    void mergedUid();
};


tst_SeasideImport::tst_SeasideImport()
{
}

QList<QContact> tst_SeasideImport::processVCard(const char *vCardData)
{
    const QByteArray ba(vCardData);
    QVersitReader reader(ba);
    if (reader.startReading() && reader.waitForFinished()) {
        return SeasideImport::buildImportContacts(reader.results());
    }

    return QList<QContact>();
}

void tst_SeasideImport::name()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:Springfield;Jebediah;;;\r\n"
"FN:\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));

    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.firstName(), QString::fromLatin1("Jebediah"));
    QCOMPARE(name.lastName(), QString::fromLatin1("Springfield"));

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 0);

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 1);

    const QContactPhoneNumber phone(phoneNumbers.at(0));
    QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
    QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
}

void tst_SeasideImport::formattedName()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:;;;;\r\n"
"FN:Jebediah Springfield\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));
    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.isEmpty(), true);

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 1);

    const QContactNickname nick(nicknames.at(0));
    QCOMPARE(nick.nickname(), QString::fromLatin1("Jebediah Springfield"));

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 1);

    const QContactPhoneNumber phone(phoneNumbers.at(0));
    QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
    QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
}

void tst_SeasideImport::nickname()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:;;;;\r\n"
"NICKNAME:Jebediah Springfield\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));
    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.isEmpty(), true);

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 1);

    const QContactNickname nick(nicknames.at(0));
    QCOMPARE(nick.nickname(), QString::fromLatin1("Jebediah Springfield"));

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 1);

    const QContactPhoneNumber phone(phoneNumbers.at(0));
    QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
    QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
}

void tst_SeasideImport::unidentified()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:;;;;\r\n"
"FN:\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));
    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.isEmpty(), true);

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 1);

    const QContactNickname nick(nicknames.at(0));
    QCOMPARE(nick.nickname(), QString::fromLatin1("555-1234"));

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 1);

    const QContactPhoneNumber phone(phoneNumbers.at(0));
    QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
    QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
}

void tst_SeasideImport::nonmergedName()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:Springfield;Jebediah;;;\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n"
"BEGIN:VCARD\r\n"
"N:Springfield;Obadiah;;;\r\n"
"TEL;TYPE=CELL:555-6789\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 2);
    {
        const QContact &c(contacts.at(0));

        const QContactName name(c.detail<QContactName>());
        QCOMPARE(name.firstName(), QString::fromLatin1("Jebediah"));
        QCOMPARE(name.lastName(), QString::fromLatin1("Springfield"));

        const QList<QContactNickname> nicknames(c.details<QContactNickname>());
        QCOMPARE(nicknames.count(), 0);

        const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
        QCOMPARE(phoneNumbers.count(), 1);

        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
    }
    {
        const QContact &c(contacts.at(1));

        const QContactName name(c.detail<QContactName>());
        QCOMPARE(name.firstName(), QString::fromLatin1("Obadiah"));
        QCOMPARE(name.lastName(), QString::fromLatin1("Springfield"));

        const QList<QContactNickname> nicknames(c.details<QContactNickname>());
        QCOMPARE(nicknames.count(), 0);

        const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
        QCOMPARE(phoneNumbers.count(), 1);

        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-6789"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeMobile));
    }
}

void tst_SeasideImport::nonmergedNickname()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"FN:Jebediah Springfield\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n"
"BEGIN:VCARD\r\n"
"NICKNAME:Obadiah Springfield\r\n"
"TEL;TYPE=CELL:555-6789\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 2);
    {
        const QContact &c(contacts.at(0));

        const QContactName name(c.detail<QContactName>());
        QCOMPARE(name.isEmpty(), true);

        const QList<QContactNickname> nicknames(c.details<QContactNickname>());
        QCOMPARE(nicknames.count(), 1);

        const QContactNickname nick(nicknames.at(0));
        QCOMPARE(nick.nickname(), QString::fromLatin1("Jebediah Springfield"));

        const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
        QCOMPARE(phoneNumbers.count(), 1);

        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
    }
    {
        const QContact &c(contacts.at(1));

        const QContactName name(c.detail<QContactName>());
        QCOMPARE(name.isEmpty(), true);

        const QList<QContactNickname> nicknames(c.details<QContactNickname>());
        QCOMPARE(nicknames.count(), 1);

        const QContactNickname nick(nicknames.at(0));
        QCOMPARE(nick.nickname(), QString::fromLatin1("Obadiah Springfield"));

        const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
        QCOMPARE(phoneNumbers.count(), 1);

        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-6789"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeMobile));
    }
}

void tst_SeasideImport::nonmergedUid()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:Springfield;Jebediah;;;\r\n"
"TEL;VOICE:555-1234\r\n"
"UID:uid-1\r\n"
"END:VCARD\r\n"
"BEGIN:VCARD\r\n"
"N:Springfield;Obadiah;;;\r\n"
"TEL;TYPE=CELL:555-6789\r\n"
"UID:uid-1\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 2);
    {
        const QContact &c(contacts.at(0));

        const QContactName name(c.detail<QContactName>());
        QCOMPARE(name.firstName(), QString::fromLatin1("Jebediah"));
        QCOMPARE(name.lastName(), QString::fromLatin1("Springfield"));

        const QList<QContactNickname> nicknames(c.details<QContactNickname>());
        QCOMPARE(nicknames.count(), 0);

        const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
        QCOMPARE(phoneNumbers.count(), 1);

        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));

        const QList<QContactGuid> guids(c.details<QContactGuid>());
        QCOMPARE(guids.count(), 1);

        const QContactGuid guid(guids.at(0));
        QCOMPARE(guid.guid(), QString::fromLatin1("uid-1"));
    }
    {
        const QContact &c(contacts.at(1));

        const QContactName name(c.detail<QContactName>());
        QCOMPARE(name.firstName(), QString::fromLatin1("Obadiah"));
        QCOMPARE(name.lastName(), QString::fromLatin1("Springfield"));

        const QList<QContactNickname> nicknames(c.details<QContactNickname>());
        QCOMPARE(nicknames.count(), 0);

        const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
        QCOMPARE(phoneNumbers.count(), 1);

        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-6789"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeMobile));

        const QList<QContactGuid> guids(c.details<QContactGuid>());
        QCOMPARE(guids.count(), 0);
    }
}

void tst_SeasideImport::mergedName()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"N:Springfield;Jebediah;;;\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n"
"BEGIN:VCARD\r\n"
"N:Springfield;Jebediah;;;\r\n"
"TEL;TYPE=CELL:555-6789\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));

    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.firstName(), QString::fromLatin1("Jebediah"));
    QCOMPARE(name.lastName(), QString::fromLatin1("Springfield"));

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 0);

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 2);
    {
        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
    }
    {
        const QContactPhoneNumber phone(phoneNumbers.at(1));
        QCOMPARE(phone.number(), QString::fromLatin1("555-6789"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeMobile));
    }
}

void tst_SeasideImport::mergedNickname()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"FN:Jebediah Springfield\r\n"
"TEL;VOICE:555-1234\r\n"
"END:VCARD\r\n"
"BEGIN:VCARD\r\n"
"NICKNAME:Jebediah Springfield\r\n"
"TEL;TYPE=CELL:555-6789\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));

    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.isEmpty(), true);

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 1);

    const QContactNickname nick(nicknames.at(0));
    QCOMPARE(nick.nickname(), QString::fromLatin1("Jebediah Springfield"));

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 2);
    {
        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
    }
    {
        const QContactPhoneNumber phone(phoneNumbers.at(1));
        QCOMPARE(phone.number(), QString::fromLatin1("555-6789"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeMobile));
    }
}

void tst_SeasideImport::mergedUid()
{
    const char *vCardData =
"BEGIN:VCARD\r\n"
"FN:Jebediah Springfield\r\n"
"TEL;VOICE:555-1234\r\n"
"UID:uid-1\r\n"
"END:VCARD\r\n"
"BEGIN:VCARD\r\n"
"FN:Obadiah Springfield\r\n"
"TEL;TYPE=CELL:555-6789\r\n"
"UID:uid-1\r\n"
"END:VCARD\r\n";

    const QList<QContact> contacts(processVCard(vCardData));
    QCOMPARE(contacts.count(), 1);

    const QContact &c(contacts.at(0));

    const QContactName name(c.detail<QContactName>());
    QCOMPARE(name.isEmpty(), true);

    const QList<QContactNickname> nicknames(c.details<QContactNickname>());
    QCOMPARE(nicknames.count(), 2);

    const QContactNickname nick1(nicknames.at(0));
    const QContactNickname nick2(nicknames.at(1));
    QVERIFY(nick1.nickname() == QString::fromLatin1("Jebediah Springfield")
            || nick1.nickname() == QString::fromLatin1("Obadiah Springfield"));
    QVERIFY(nick2.nickname() == QString::fromLatin1("Jebediah Springfield")
            || nick2.nickname() == QString::fromLatin1("Obadiah Springfield"));
    QVERIFY(nick1.nickname() != nick2.nickname());

    const QList<QContactPhoneNumber> phoneNumbers(c.details<QContactPhoneNumber>());
    QCOMPARE(phoneNumbers.count(), 2);
    {
        const QContactPhoneNumber phone(phoneNumbers.at(0));
        QCOMPARE(phone.number(), QString::fromLatin1("555-1234"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeVoice));
    }
    {
        const QContactPhoneNumber phone(phoneNumbers.at(1));
        QCOMPARE(phone.number(), QString::fromLatin1("555-6789"));
        QCOMPARE(phone.subTypes(), QList<int>() << static_cast<int>(QContactPhoneNumber::SubTypeMobile));
    }

    const QList<QContactGuid> guids(c.details<QContactGuid>());
    QCOMPARE(guids.count(), 1);

    const QContactGuid guid(guids.at(0));
    QCOMPARE(guid.guid(), QString::fromLatin1("uid-1"));
}

#include "tst_seasideimport.moc"
QTEST_GUILESS_MAIN(tst_SeasideImport)
