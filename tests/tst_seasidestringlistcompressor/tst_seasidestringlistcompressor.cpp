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

    QTest::newRow("Compress 5 from 6, compress 1 then 2 to complete")
            << QStringList({"a", "b", "c", "d", "e", "f"}) << 5
            << QStringList({"a", ".", "c", ".", "f"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b"}) },
                    { 3, QStringList({"d", "e"}) }
               };

    QTest::newRow("Compress 5 from 7, keep 'd' centre symbol and compress 2 sets on either side")
            << QStringList({"a", "b", "c", "d", "e", "f", "g"}) << 5
            << QStringList({"a", ".", "d", ".", "g"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f"}) }
               };

    QTest::newRow("Compress 5 from 8, compress 2 before 'd', then 3 after")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h"}) << 5
            << QStringList({"a", ".", "d", ".", "h"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f", "g"}) }
               };

    QTest::newRow("Compress 5 from 9, compress 3 before e, then 3 after")
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

    QTest::newRow("Compress 10 from 16, results in count=9")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"}) << 10
            << QStringList({"a", ".", "d", ".", "h", ".", "l", ".", "p"})
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

    QTest::newRow("Compress 10 from 18, results in count=9")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r"}) << 10
            << QStringList({"a", ".", "d", ".", "i", ".", "n", ".", "r"})
            << SeasideStringListCompressor::CompressedContent {
                    { 1, QStringList({"b", "c"}) },
                    { 3, QStringList({"e", "f", "g", "h"}) },
                    { 5, QStringList({"j", "k", "l", "m"}) },
                    { 7, QStringList({"o", "p", "q"}) }
               };

    QTest::newRow("Compress 10 from 19, results in count=9. First and last groups are balanced.")
            << QStringList({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s"}) << 10
            << QStringList({"a", ".", "e", ".", "j", ".", "o", ".", "s"})
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
}

void tst_SeasideStringListCompressor::tst_compress()
{
    QFETCH(QStringList, inputList);
    QFETCH(int, compressTargetSize);
    QFETCH(QStringList, expected);
    QFETCH(SeasideStringListCompressor::CompressedContent, expectedCompressedContents);

    SeasideStringListCompressor::CompressedContent compressedContents;
    QCOMPARE(SeasideStringListCompressor::compress(inputList, compressTargetSize, &compressedContents), expected);

    QCOMPARE(compressedContents, expectedCompressedContents);
}

#include "tst_seasidestringlistcompressor.moc"
QTEST_APPLESS_MAIN(tst_SeasideStringListCompressor)
