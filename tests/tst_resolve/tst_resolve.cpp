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

#include <QObject>
#include <QtTest>
#include <QtDebug>

#include <QContact>
#include <QContactEmailAddress>
#include <QContactName>
#include <QContactOnlineAccount>
#include <QContactPhoneNumber>

#include "seasidecache.h"

static const QString AccountPath(QString::fromLatin1("/example/jabber/0"));

class tst_Resolve : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private:
    // only firstname and lastname are mandatory
    bool makeContact(QString firstname, QString lastname, QString phone, QString email, QString account);
    void makeContacts();

    QList<QContactId> m_createdContacts;

private slots:
    void resolveByPhone_data();
    void resolveByPhone();
    void resolveByPhoneNotFound_data();
    void resolveByPhoneNotFound();
    void resolveByEmail();
    void resolveByEmailNotFound();
    void resolveByAccount();
    void resolveByAccountNotFound();

    void resolveDuringContactLink();
};

namespace {
struct TestResolveListener : public SeasideCache::ResolveListener {
    TestResolveListener()
        : m_resolved(false), m_item(0)
        { }

    virtual void addressResolved(const QString &, const QString &, SeasideCache::CacheItem *item)
        { m_resolved = true; m_item = item; }

    bool m_resolved;
    SeasideCache::CacheItem *m_item;
};

} // anonymous

void tst_Resolve::initTestCase()
{
    makeContacts();
    SeasideCache::registerUser(this);
}

void tst_Resolve::cleanupTestCase()
{
    SeasideCache::unregisterUser(this);
    QVERIFY(SeasideCache::manager()->removeContacts(m_createdContacts));
    m_createdContacts.clear();
}

bool tst_Resolve::makeContact(QString firstname, QString lastname, QString phone, QString email, QString account)
{
    QContact contact;

    QContactName name;
    name.setFirstName(firstname);
    name.setLastName(lastname);
    if (!contact.saveDetail(&name))
        return false;

    if (!phone.isEmpty()) {
        QContactPhoneNumber pn;
        pn.setNumber(phone);
        if (!contact.saveDetail(&pn))
            return false;
    }

    if (!email.isEmpty()) {
        QContactEmailAddress e;
        e.setEmailAddress(email);
        if (!contact.saveDetail(&e))
            return false;
    }

    if (!account.isEmpty()) {
        QContactOnlineAccount acc;
        acc.setAccountUri(account);
        acc.setValue(QContactOnlineAccount__FieldAccountPath, AccountPath);
        if (!contact.saveDetail(&acc))
            return false;
    }

    if (!SeasideCache::manager()->saveContact(&contact))
        return false;

    m_createdContacts << contact.id();
    return true;
}

void tst_Resolve::makeContacts()
{
    QVERIFY(makeContact("Alfred", "Alfredson", "+358474005000", "alfred@alfred.com", ""));
    QVERIFY(makeContact("Berta", "Berenstain", "", "berta.b@geemail.com", "berta.b@geemail.com"));
    QVERIFY(makeContact("Carlo", "Rizzi", "+358471112222", "", ""));
    QVERIFY(makeContact("Daffy", "Duck", "+358470009955", "daffyd@example.com", ""));
    QVERIFY(makeContact("Dafferd", "Duck", "", "daffy.d@example.com", ""));
    QVERIFY(makeContact("Ernest", "Everest", "+358477758885", "", ""));
    QVERIFY(makeContact("John", "Smith", "+36701234567", "", ""));
    QVERIFY(makeContact("Jane", "Smith", "06207654321", "", ""));
}

void tst_Resolve::resolveByPhone_data()
{
    QTest::addColumn<QString>("country");
    QTest::addColumn<QString>("number");
    QTest::addColumn<QString>("matchingName");

    QTest::newRow("Daffy international")
            << "FI" << "+358470009955" << "Daffy";
    QTest::newRow("Daffy international direct dial")
            << "FI" << "00358470009955" << "Daffy";

    QTest::newRow("John international")
            << "HU" << "+36701234567" << "John";
    QTest::newRow("John international direct dial")
            << "HU" << "0036701234567" << "John";
    QTest::newRow("John international without plus")
            << "HU" << "36701234567" << "John";
    QTest::newRow("John full national")
            << "HU" << "06701234567" << "John";

    QTest::newRow("Jane international")
            << "HU" << "+36207654321" << "Jane";
    QTest::newRow("Jane international without plus")
            << "HU" << "36207654321" << "Jane";
    QTest::newRow("Jane full national")
            << "HU" << "06207654321" << "Jane";
}

void tst_Resolve::resolveByPhone()
{
    // Note:
    // When running these tests, make sure you don't have any of the above
    // numbers in your contact list, as those will interfere with the results.

    QFETCH(QString, country);
    QFETCH(QString, number);
    QFETCH(QString, matchingName);
    Q_UNUSED(country)

    SeasideCache::CacheItem *item;
    TestResolveListener listener;

    item = SeasideCache::resolvePhoneNumber(&listener, number, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    }

    QVERIFY(item != 0);

    QContactName name = item->contact.detail<QContactName>();
    QCOMPARE(name.firstName(), matchingName);
}

void tst_Resolve::resolveByPhoneNotFound_data()
{
    QTest::addColumn<QString>("country");
    QTest::addColumn<QString>("number");

    QTest::newRow("Non-existent British number")
            << "GB" << "+44123456789";
    QTest::newRow("Non-existent Finnish number")
            << "FI" << "+35898765432100";

    QTest::newRow("Daffy same NSN different country")
            << "HU" << "+36470009955";
    QTest::newRow("Daffy same number same country different area code")
            << "FI" << "+358570009955";

    QTest::newRow("John same NSN different country")
            << "HU" << "+44701234567";
    QTest::newRow("John same number same country different area code")
            << "HU" << "+36201234567";

    QTest::newRow("Jane same number different area code international")
            << "HU" << "+36307654321";
    QTest::newRow("Jane same number different area code full national")
            << "HU" << "06307654321";
}

void tst_Resolve::resolveByPhoneNotFound()
{
    // Note:
    // When running these tests, make sure you don't have any of the above
    // numbers in your contact list, as those will interfere with the results.

    QFETCH(QString, country);
    QFETCH(QString, number);
    Q_UNUSED(country)

    SeasideCache::CacheItem *item;
    TestResolveListener listener;

    item = SeasideCache::resolvePhoneNumber(&listener, number, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    } else {
        qWarning() << "should not have resolved:" << number << "to" << item->displayLabel;
    }

    QCOMPARE(item, (SeasideCache::CacheItem *)0);
}

void tst_Resolve::resolveByEmail()
{
    SeasideCache::CacheItem *item;
    TestResolveListener listener;
    QString address("berta.b@geemail.com");

    item = SeasideCache::resolveEmailAddress(&listener, address, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    }

    QContactName name = item->contact.detail<QContactName>();
    QCOMPARE(name.firstName(), QString::fromLatin1("Berta"));
}

void tst_Resolve::resolveByEmailNotFound()
{
    SeasideCache::CacheItem *item;
    TestResolveListener listener;
    QString address("example@example.com");

    item = SeasideCache::resolveEmailAddress(&listener, address, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    }

    QCOMPARE(item, (SeasideCache::CacheItem *)0);
}

void tst_Resolve::resolveByAccount()
{
    SeasideCache::CacheItem *item;
    TestResolveListener listener;
    QString remoteUid("berta.b@geemail.com");

    item = SeasideCache::resolveOnlineAccount(&listener, AccountPath, remoteUid, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    }

    QContactName name = item->contact.detail<QContactName>();
    QCOMPARE(name.firstName(), QString::fromLatin1("Berta"));
}

void tst_Resolve::resolveByAccountNotFound()
{
    SeasideCache::CacheItem *item;
    TestResolveListener listener;
    QString remoteUid("example@example.com");

    item = SeasideCache::resolveOnlineAccount(&listener, AccountPath, remoteUid, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    }

    QCOMPARE(item, (SeasideCache::CacheItem *)0);
}

struct ItemWatcher : public SeasideCache::ItemData {
    QList<int> m_constituents;
    bool m_aggregationComplete;

    ItemWatcher() : m_aggregationComplete(false)
    { }

    virtual void displayLabelOrderChanged(SeasideCache::DisplayLabelOrder)
    { }
    virtual void updateContact(const QtContacts::QContact&, QtContacts::QContact*, SeasideCache::ContactState)
    { }
    virtual void mergeCandidatesFetched(const QList<int> &)
    { }

    virtual void aggregationOperationCompleted()
    { m_aggregationComplete = true; }

    virtual void constituentsFetched(const QList<int> &ids)
    { m_constituents = ids; }

    virtual QList<int> constituents() const
    { return m_constituents; }
};

// Test that address resolutions don't interfere with contact linking
void tst_Resolve::resolveDuringContactLink()
{
    SeasideCache::CacheItem *item1;
    TestResolveListener listener1;
    QString address1("daffyd@example.com");
    SeasideCache::CacheItem *item2;
    TestResolveListener listener2;
    QString address2("daffy.d@example.com");

    item1 = SeasideCache::resolveEmailAddress(&listener1, address1, true);
    item2 = SeasideCache::resolveEmailAddress(&listener2, address2, true);
    if (!item1) {
        QTRY_VERIFY(listener1.m_resolved);
        item1 = listener1.m_item;
    }
    QVERIFY(item1);
    QCOMPARE(item1->displayLabel, QString::fromLatin1("Daffy Duck"));
    if (!item2) {
        QTRY_VERIFY(listener2.m_resolved);
        item2 = listener2.m_item;
    }
    QVERIFY(item2);
    QCOMPARE(item2->displayLabel, QString::fromLatin1("Dafferd Duck"));

    int iid = item1->iid;
    item1->itemData = new ItemWatcher;
    item2->itemData = new ItemWatcher;
    SeasideCache::aggregateContacts(item1->contact, item2->contact);

    // Fire off an address resolution simultaneously
    SeasideCache::CacheItem *item;
    QString number("+358477758885");
    TestResolveListener listener;

    item = SeasideCache::resolvePhoneNumber(&listener, number, true);
    if (!item) {
        QTRY_VERIFY(listener.m_resolved);
        item = listener.m_item;
    }

    // Did the address resolution go ok?
    QCOMPARE(item->displayLabel, QString::fromLatin1("Ernest Everest"));

    // wait for the aggregation
    item1 = SeasideCache::existingItem(iid);
    QVERIFY(item1);
    QVERIFY(item1->itemData);
    QTRY_VERIFY(((ItemWatcher *) item1->itemData)->m_aggregationComplete);

    // the aggregate's constituents are not updated in the cache, so they
    // have to be reloaded before comparing. Is that a bug?
    SeasideCache::fetchConstituents(item1->contact);
    QTRY_COMPARE(item1->itemData->constituents().count(), 2);

    // Check that the expected two contacts are the constituents.
    int iid1 = item1->itemData->constituents()[0];
    int iid2 = item1->itemData->constituents()[1];
    item1 = SeasideCache::itemById(iid1);
    item2 = SeasideCache::itemById(iid2);
    QTRY_COMPARE(item1->contactState, SeasideCache::ContactComplete);
    QTRY_COMPARE(item2->contactState, SeasideCache::ContactComplete);

    QStringList names, expected;
    names << item1->displayLabel << item2->displayLabel;
    std::sort(names.begin(), names.end());
    expected << QString::fromLatin1("Dafferd Duck") << QString::fromLatin1("Daffy Duck");
    QCOMPARE(names, expected);
}

#include "tst_resolve.moc"
QTEST_GUILESS_MAIN(tst_Resolve)
