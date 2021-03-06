/*
 * Copyright (c) 2013 - 2020 Jolla Ltd.
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

#include "synchronizelists.h"


class tst_SynchronizeLists : public QObject
{
    Q_OBJECT

public:
    tst_SynchronizeLists();

    bool m_filterEnabled;
    QVector<quint32> m_filter;
    QVector<quint32> m_cache;

    bool filterValue(quint32 contactId) const;
    int insertRange(int index, int count, const QVector<quint32> &source, int sourceIndex);
    int removeRange(int index, int count);

private slots:
    void filtered_data();
    void filtered();
    void unfiltered_data();
    void unfiltered();
};

typedef QVector<quint32> List;

Q_DECLARE_METATYPE(List)


tst_SynchronizeLists::tst_SynchronizeLists()
{
    qRegisterMetaType<List>();
}

bool tst_SynchronizeLists::filterValue(quint32 contactId) const
{
    return !m_filterEnabled || m_filter.contains(contactId);
}

int tst_SynchronizeLists::insertRange(
        int index, int count, const QVector<quint32> &source, int sourceIndex)
{
    for (int i = 0; i < count; ++i)
        m_cache.insert(index + i, source.at(sourceIndex + i));

    return count;
}

int tst_SynchronizeLists::removeRange(int index, int count)
{
    m_cache.remove(index, count);

    return 0;
}

void tst_SynchronizeLists::filtered_data()
{
    QTest::addColumn<QVector<quint32> >("reference");
    QTest::addColumn<QVector<quint32> >("original");
    QTest::addColumn<QVector<quint32> >("expected");

    {
        const List reference = List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10;
        const List original  = List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10;
        QTest::newRow("a0")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("a1")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5);
        QTest::newRow("a2")
                << reference
                << original
                << (List() << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("a3")
                << reference
                << original
                << (List() << 1 << 3 << 5 << 7 << 9);
        QTest::newRow("a4")
                << reference
                << original
                << (List() << 0 << 2 << 4 << 6 << 8 << 10);
        QTest::newRow("a5")
                << reference
                << original
                << (List() << 0 << 1 << 3 << 8 << 9 << 10);
        QTest::newRow("a6")
                << reference
                << original
                << (List() << 4 << 5 << 6 << 10);
    } {
        const List reference = List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10;
        const List original  = List();

        QTest::newRow("b0")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("b1")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5);
        QTest::newRow("b2")
                << reference
                << original
                << (List() << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("b3")
                << reference
                << original
                << (List() << 1 << 3 << 5 << 7 << 9);
        QTest::newRow("b4")
                << reference
                << original
                << (List() << 0 << 2 << 4 << 6 << 8 << 10);
        QTest::newRow("b5")
                << reference
                << original
                << (List() << 0 << 1 << 3 << 8 << 9 << 10);
        QTest::newRow("b6")
                << reference
                << original
                << (List() << 4 << 5 << 6 << 10);
    } {
        const List reference = List() << 0  << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10;
        const List original  = List() << 10 << 9 << 8 << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0;

        QTest::newRow("c0")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("c1")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5);
        QTest::newRow("c2")
                << reference
                << original
                << (List() << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("c3")
                << reference
                << original
                << (List() << 1 << 3 << 5 << 7 << 9);
        QTest::newRow("c4")
                << reference
                << original
                << (List() << 0 << 2 << 4 << 6 << 8 << 10);
        QTest::newRow("c5")
                << reference
                << original
                << (List() << 0 << 1 << 3 << 8 << 9 << 10);
        QTest::newRow("c6")
                << reference
                << original
                << (List() << 4 << 5 << 6 << 10);
    } {
        const List reference = List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10;
        const List original  = List() << 2 << 3 << 4 << 7 << 8;

        QTest::newRow("d0")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("d1")
                << reference
                << original
                << (List() << 0 << 1 << 2 << 3 << 4 << 5);
        QTest::newRow("d2")
                << reference
                << original
                << (List() << 6 << 7 << 8 << 9 << 10);
        QTest::newRow("d3")
                << reference
                << original
                << (List() << 1 << 3 << 5 << 7 << 9);
        QTest::newRow("d4")
                << reference
                << original
                << (List() << 0 << 2 << 4 << 6 << 8 << 10);
        QTest::newRow("d5")
                << reference
                << original
                << (List() << 0 << 1 << 3 << 8 << 9 << 10);
        QTest::newRow("d6")
                << reference
                << original
                << (List() << 4 << 5 << 6 << 10);
    }
}

void tst_SynchronizeLists::filtered()
{
    QFETCH(QVector<quint32>, reference);
    QFETCH(QVector<quint32>, original);
    QFETCH(QVector<quint32>, expected);

    m_filterEnabled = true;
    m_cache = original;
    m_filter = expected;

    synchronizeFilteredList(this, m_cache, reference);

    if (m_cache != expected) {
        qDebug() << "expected" << expected;
        qDebug() << "actual  " << m_cache;
    }

    QCOMPARE(m_cache, expected);
}

void tst_SynchronizeLists::unfiltered_data()
{
    QTest::addColumn<QVector<quint32> >("reference");
    QTest::addColumn<QVector<quint32> >("original");

    QTest::newRow("0")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List());
    QTest::newRow("1")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10);
    QTest::newRow("2")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List() << 0 << 1 << 2 << 3 << 4 << 5);
    QTest::newRow("3")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List() << 6 << 7 << 8 << 9 << 10);
    QTest::newRow("4")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List() << 10 << 9 << 8 << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
    QTest::newRow("5")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List() << 5 << 4 << 3 << 2 << 1 << 0);
    QTest::newRow("6")
            << (List() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10)
            << (List() << 10 << 9 << 8 << 7 << 6);
}

void tst_SynchronizeLists::unfiltered()
{
    QFETCH(QVector<quint32>, reference);
    QFETCH(QVector<quint32>, original);

    m_filterEnabled = false;
    m_cache = original;

    synchronizeList(this, m_cache, reference);

    if (m_cache != reference) {
        qDebug() << "expected" << reference;
        qDebug() << "actual  " << m_cache;
    }

    QCOMPARE(m_cache, reference);
}

#include "tst_synchronizelists.moc"
QTEST_APPLESS_MAIN(tst_SynchronizeLists)
