/*
 * Copyright (C) 2019 Jolla Mobile <bea.lam@jollamobile.com>
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

#include "seasidestringlistcompressor.h"

class tst_SeasideStringListCompressor : public QObject
{
    Q_OBJECT
public:
    tst_SeasideStringListCompressor() {}

private slots:
    void tst_compressedSymbolList_data();
    void tst_compressedSymbolList();
};


void tst_SeasideStringListCompressor::tst_compressedSymbolList_data()
{
    QTest::addColumn<QStringList>("inputList");
    QTest::addColumn<int>("compressTargetSize");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("No compression, size less than minimum")
            << QStringList({"a", "b", "c"}) << 5
            << QStringList({"a", "b", "c"});

    QTest::newRow("No compression, target size same as input size")
            << QStringList({"a", "b", "c", "d", "e"}) << 5
            << QStringList({"a", "b", "c", "d", "e"});

    QTest::newRow("Compress 5 from 6, compress 1 set of centre symbols")
            << QStringList({"a", "b", "c", "d", "e", "f"}) << 5
            << QStringList({"a", "b", ".", "e", "f"});

    QTest::newRow("Compress 5 from 7, keep 'd' centre symbol and compress 2 sets on either side")
            << QStringList({"a", "b", "c", "d", "e", "f", "g"}) << 5
            << QStringList({"a", ".", "d", ".", "g"});

    QTest::newRow("Compress 5 from 8, compress 4 centre symbols since b & g can't be compressed")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h"}) << 5
            << QStringList({"a", "b", ".", "g", "h"});

    QTest::newRow("Compress 5 from 9, keep 'e' centre symbol and compress all on either side")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i"}) << 5
            << QStringList({"a", ".", "e", ".", "i"});

    QTest::newRow("Compress 5 from 10, requires repeat loops back to centre to reset")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"}) << 5
            << QStringList({"a", ".", "g", ".", "j"});

    QTest::newRow("Compress 5 from 11")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k"}) << 5
            << QStringList({"a", ".", "f", ".", "k"});

    QTest::newRow("Compress 5 from 12")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l"}) << 5
            << QStringList({"a", ".", "h", ".", "l"});

    QTest::newRow("Compress 10 from 15")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"}) << 10
            << QStringList({"a", ".", "e", ".", "h", ".", "k", ".", "n", "o"});

    QTest::newRow("Compress 10 from 16, results in count=9")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"}) << 10
            << QStringList({"a", ".", "g", ".", "j", ".", "m", ".", "p"});

    QTest::newRow("Compress 10 from 17")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"}) << 10
            << QStringList({"a", ".", "f", ".", "i", ".", "l", ".", "p", "q"});

    QTest::newRow("Compress 10 from 18, results in count=9")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 10
            << QStringList({"a", ".", "h", ".", "k", ".", "n", ".", "r"});

    QTest::newRow("Compress 10 from 19, results in count=9")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s"}) << 10
            << QStringList({"a", ".", "g", ".", "j", ".", "m", ".", "s"});

    QTest::newRow("Compress 10 from 20")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t"}) << 10
            << QStringList({"a", ".", "i", ".", "l", ".", "o", ".", "t"});

    // Compress latin alphabet plus 2 symbols (e.g. #, !)
    QTest::newRow("Compress 10 from 26 + symbols")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "!", "#"}) << 10
            << QStringList({"a", ".", "m", ".", "p", ".", "v", ".", "#"});
}

void tst_SeasideStringListCompressor::tst_compressedSymbolList()
{
    QFETCH(QStringList, inputList);
    QFETCH(int, compressTargetSize);
    QFETCH(QStringList, expected);

    QCOMPARE(SeasideStringListCompressor::compress(inputList, compressTargetSize), expected);
}

#include "tst_seasidestringlistcompressor.moc"
QTEST_APPLESS_MAIN(tst_SeasideStringListCompressor)
