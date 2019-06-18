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

#ifndef SEASIDESTRINGLISTCOMPRESSOR_H
#define SEASIDESTRINGLISTCOMPRESSOR_H

#include <QStringList>
#include <QMap>
#include <QMetaType>

class SeasideStringListCompressor
{
public:
    SeasideStringListCompressor();
    ~SeasideStringListCompressor();

    typedef QMap<int, QStringList> CompressedContent;

    static QStringList compress(const QStringList &strings, int desiredSize, CompressedContent *compressedSections);
    static bool isCompressionMarker(const QString &s);
    static int minimumCompressionInputCount();

private:
    struct SectionsInfo {
        int maxSize = 0;
        int minSize = 0;
        int countMaxSized = 0;
        int countMinSized = 0;
    };

    static QStringList minimalCompress(const QStringList &strings, int desiredSize, CompressedContent *compressedSections);
    static QStringList accordionCompress(const QStringList &strings, int desiredSize, CompressedContent *compressedSections);
    static QStringList performCompression(const QStringList &strings,
                                          SectionsInfo uncompressedSectionsInfo,
                                          SectionsInfo compressedSectionsInfo,
                                          CompressedContent *compressedSections);
    static int compressionMarkerCount(int compressTargetCount);
};

Q_DECLARE_METATYPE(SeasideStringListCompressor::CompressedContent)

#endif
