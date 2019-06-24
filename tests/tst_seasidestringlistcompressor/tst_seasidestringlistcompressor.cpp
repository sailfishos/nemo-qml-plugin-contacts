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
    void tst_compress();
    void tst_compress_data();
};

void tst_SeasideStringListCompressor::tst_compress_data()
{
    QTest::addColumn<QStringList>("inputList");
    QTest::addColumn<int>("compressTargetSize");
    QTest::addColumn<QStringList>("expected");
    QTest::addColumn<SeasideStringListCompressor::CompressedContent>("expectedCompressedContents");

    QTest::newRow("No compression, size less than minimum")
            << QStringList({"a", "b", "c"}) << 5
            << QStringList({"a", "b", "c"})
            << SeasideStringListCompressor::CompressedContent();

    QTest::newRow("No compression, target size same as input size")
            << QStringList({"a", "b", "c", "d", "e"}) << 5
            << QStringList({"a", "b", "c", "d", "e"})
            << SeasideStringListCompressor::CompressedContent();

    QTest::newRow("Compress 5 from 6")
            << QStringList({"a", "b", "c", "d", "e", "f"}) << 5
            << QStringList({"a", "b", ".", "e", "f"})
            << SeasideStringListCompressor::CompressedContent {
                    { 2, QStringList({"c", "d"}) },
               };

    QTest::newRow("Compress 5 from 7")
            << QStringList({"a", "b", "c", "d", "e", "f", "g"}) << 5
            << QStringList({"a", ".", "d", ".", "g"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f"}) }
               };

    QTest::newRow("Compress 5 from 8")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h"}) << 5
            << QStringList({"a", ".", "d", ".", "h"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f", "g"}) }
               };

    QTest::newRow("Compress 5 from 9")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i"}) << 5
            << QStringList({"a", ".", "e", ".", "i"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d"}) },
                    { 3, QStringList({"f", "g", "h"}) }
               };

    QTest::newRow("Compress 5 from 10")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"}) << 5
            << QStringList({"a", ".", "e", ".", "j"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d"}) },
                    { 3, QStringList({"f", "g", "h", "i"}) }
               };

    QTest::newRow("Compress 5 from 11")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k"}) << 5
            << QStringList({"a", ".", "f", ".", "k"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d", "e"}) },
                    { 3, QStringList({"g", "h", "i", "j"}) }
               };

    QTest::newRow("Compress 5 from 12")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l"}) << 5
            << QStringList({"a", ".", "f", ".", "l"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d", "e"}) },
                    { 3, QStringList({"g", "h", "i", "j", "k"}) }
               };

    QTest::newRow("Compress 10 from 15")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"}) << 10
            << QStringList({"a", ".", "d", ".", "h", ".", "l", ".", "o"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f", "g"}) },
                    { 5, QStringList({"i", "j", "k"}) },
                    { 7, QStringList({"m", "n"}) }
               };

    QTest::newRow("Compress 10 from 16")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"}) << 10
            << QStringList({"a", ".", "d", ".", "h", ".", "l", ".", "p"}) // note: expectedCount = 9, not 10.
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f", "g"}) },
                    { 5, QStringList({"i", "j", "k"}) },
                    { 7, QStringList({"m", "n", "o"}) }
               };

    QTest::newRow("Compress 10 from 17")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"}) << 10
            << QStringList({"a", ".", "e", ".", "i", ".", "m", ".", "q"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d"}) },
                    { 3, QStringList({"f", "g", "h"}) },
                    { 5, QStringList({"j", "k", "l"}) },
                    { 7, QStringList({"n", "o", "p"}) }
               };

    QTest::newRow("Compress 10 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 10
            << QStringList({"a", ".", "e", ".", "i", ".", "n", ".", "r"}) // note: expectedCount = 9, not 10.
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d"}) },
                    { 3, QStringList({"f", "g", "h"}) },
                    { 5, QStringList({"j", "k", "l", "m"}) },
                    { 7, QStringList({"o", "p", "q"}) }
               };

    QTest::newRow("Compress 10 from 19")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s"}) << 10
            << QStringList({"a", ".", "e", ".", "j", ".", "o", ".", "s"}) // note: expectedCount = 9, not 10.  first+last groups are balanced.
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d"}) },
                    { 3, QStringList({"f", "g", "h", "i"}) },
                    { 5, QStringList({"k", "l", "m", "n"}) },
                    { 7, QStringList({"p", "q", "r"}) }
               };

    QTest::newRow("Compress 10 from 20")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t"}) << 10
            << QStringList({"a", ".", "e", ".", "j", ".", "o", ".", "t"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d"}) },
                    { 3, QStringList({"f", "g", "h", "i"}) },
                    { 5, QStringList({"k", "l", "m", "n"}) },
                    { 7, QStringList({"p", "q", "r", "s"}) }
               };

    QTest::newRow("Compress 14 from 26")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"}) << 14
            << QStringList({"a", ".", "e", ".", "i", ".", "m", ".", "r", ".", "v", ".", "z"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1,  QStringList({"b", "c", "d"}) },
                    { 3,  QStringList({"f", "g", "h"}) },
                    { 5,  QStringList({"j", "k", "l"}) },
                    { 7,  QStringList({"n", "o", "p", "q"}) },
                    { 9,  QStringList({"s", "t", "u"}) },
                    { 11, QStringList({"w", "x", "y"}) }
               };

    // Compress latin alphabet plus 2 symbols (e.g. #, !)
    QTest::newRow("Compress 10 from 26 + symbols")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "!", "#"}) << 10
            << QStringList({"a", ".", "g", ".", "n", ".", "u", ".", "#"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c", "d", "e", "f"}) },
                    { 3, QStringList({"h", "i", "j", "k", "l", "m"}) },
                    { 5, QStringList({"o", "p", "q", "r", "s", "t"}) },
                    { 7, QStringList({"v", "w", "x", "y", "z", "!"}) }
               };

    // 15->14: abcdefghijklmno -> abcdefg.jklmno
    QTest::newRow("Compress 14 from 15")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"}) << 14
            << QStringList({"a", "b", "c", "d", "e", "f", "g", ".", "j", "k", "l", "m", "n", "o"})
            << SeasideStringListCompressor::CompressedContent {
                    { 7,  QStringList({"h", "i"}) },
               };

    // 15->13: abcdefghijklmno -> abcd.ghi.lmno
    QTest::newRow("Compress 13 from 15")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"}) << 13
            << QStringList({"a", "b", "c", "d", ".", "g", "h", "i", ".", "l", "m", "n", "o"})
            << SeasideStringListCompressor::CompressedContent {
                    { 4,  QStringList({"e", "f"}) },
                    { 8,  QStringList({"j", "k"}) },
               };

    // 15->12: abcdefghijklmno -> ab.ef.ijk.no
    QTest::newRow("Compress 12 from 15")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"}) << 12
            << QStringList({"a", "b", "c", ".", "f", "g", ".", "j", "k", ".", "n", "o"})
            << SeasideStringListCompressor::CompressedContent {
                    { 3,  QStringList({"d", "e"}) },
                    { 6,  QStringList({"h", "i"}) },
                    { 9,  QStringList({"l", "m"}) },
               };

    // 15->11: abcdefghijklmno -> ab.e.h.k.no
    QTest::newRow("Compress 11 from 15")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"}) << 11
            << QStringList({"a", "b", ".", "e", ".", "h", ".", "k", ".", "n", "o"})
            << SeasideStringListCompressor::CompressedContent {
                    { 2,  QStringList({"c", "d"}) },
                    { 4,  QStringList({"f", "g"}) },
                    { 6,  QStringList({"i", "j"}) },
                    { 8,  QStringList({"l", "m"}) },
               };

    // 18->17: abcdefghijklmnopqr -> abcdefgh.klmnopqr
    QTest::newRow("Compress 17 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 17
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", ".", "k", "l", "m", "n", "o", "p", "q", "r"})
            << SeasideStringListCompressor::CompressedContent {
                    { 8,  QStringList({"i", "j"}) },
               };

    // 18->16: abcdefghijklmnopqr -> abcde.hijk.nopqr
    QTest::newRow("Compress 16 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 16
            << QStringList({"a", "b", "c", "d", "e", ".", "h", "i", "j", "k", ".", "n", "o", "p", "q", "r"})
            << SeasideStringListCompressor::CompressedContent {
                    { 5,  QStringList({"f", "g"}) },
                    { 10, QStringList({"l", "m"}) },
               };

    // 18->15: abcdefghijklmnopqr -> abc.fgh.klm.pqr
    QTest::newRow("Compress 15 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 15
            << QStringList({"a", "b", "c", ".", "f", "g", "h", ".", "k", "l", "m", ".", "p", "q", "r"})
            << SeasideStringListCompressor::CompressedContent {
                    { 3,  QStringList({"d", "e"}) },
                    { 7,  QStringList({"i", "j"}) },
                    { 11, QStringList({"n", "o"}) },
               };

    // 18->14: abcdefghijklmnopqr -> ab.ef.ij.mn.qr
    QTest::newRow("Compress 14 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 14
            << QStringList({"a", "b", ".", "e", "f", ".", "i", "j", ".", "m", "n", ".", "q", "r"})
            << SeasideStringListCompressor::CompressedContent {
                    { 2,  QStringList({"c", "d"}) },
                    { 5,  QStringList({"g", "h"}) },
                    { 8,  QStringList({"k", "l"}) },
                    { 11, QStringList({"o", "p"}) },
               };

    // 18->13: abcdefghijklmnopqr -> ab.e.h.k.n.qr
    QTest::newRow("Compress 13 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 13
            << QStringList({"a", "b", ".", "e", ".", "h", ".", "k", ".", "n", ".", "q", "r"})
            << SeasideStringListCompressor::CompressedContent {
                    { 2,  QStringList({"c", "d"}) },
                    { 4,  QStringList({"f", "g"}) },
                    { 6,  QStringList({"i", "j"}) },
                    { 8,  QStringList({"l", "m"}) },
                    { 10, QStringList({"o", "p"}) },
               };

    // 13->7: abcdefghijklm -> a.e.i.m accordion compression
    QTest::newRow("Compress 7 from 13")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m"}) << 7
            << QStringList({"a", ".", "e", ".", "i", ".", "m"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1,  QStringList({"b", "c", "d"}) },
                    { 3,  QStringList({"f", "g", "h"}) },
                    { 5,  QStringList({"j", "k", "l"}) },
               };

    // 18->12: abcdefghijklmnopqr -> a.d.g.k.o.r accordion compression
    QTest::newRow("Compress 12 from 18")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 12
            << QStringList({"a", ".", "d", ".", "g", ".", "k", ".", "o", ".", "r"}) // expected size is 11 not 12
            << SeasideStringListCompressor::CompressedContent {
                    { 1,  QStringList({"b", "c"}) },
                    { 3,  QStringList({"e", "f"}) },
                    { 5,  QStringList({"h", "i", "j"}) },
                    { 7,  QStringList({"l", "m", "n"}) },
                    { 9,  QStringList({"p", "q"}) },
               };
}

void tst_SeasideStringListCompressor::tst_compress()
{
    QFETCH(QStringList, inputList);
    QFETCH(int, compressTargetSize);
    QFETCH(QStringList, expected);
    QFETCH(SeasideStringListCompressor::CompressedContent, expectedCompressedContents);

    SeasideStringListCompressor::CompressedContent compressedContents;
    const QStringList result = SeasideStringListCompressor::compress(inputList, compressTargetSize, &compressedContents);

    if (result != expected || compressedContents != expectedCompressedContents) {
        qWarning() << " input:" << inputList;
        qWarning() << "actual:" << result;
        qWarning() << "expect:" << expected;
        qWarning() << "  a.cc:" << compressedContents;
        qWarning() << "  e.cc:" << expectedCompressedContents;
    }

    QCOMPARE(result, expected);
    QCOMPARE(compressedContents, expectedCompressedContents);
}

#include "tst_seasidestringlistcompressor.moc"
QTEST_APPLESS_MAIN(tst_SeasideStringListCompressor)
