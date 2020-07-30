/*
 * Copyright (c) 2019 - 2020 Jolla Ltd.
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

#include "seasidestringlistcompressor.h"

#include <QDebug>

namespace {

// The implementaiton requires the CompressionMarker to be distinct from any
// group display labels. Most labels are single character so won't conflict,
// but that's not a requirement and so display label grouper plugins must avoid
// using ".." as a label.
const QString CompressionMarker = QStringLiteral("..");
const int MinimumCompressionInputCount = 5;

}

SeasideStringListCompressor::SeasideStringListCompressor()
{
}

SeasideStringListCompressor::~SeasideStringListCompressor()
{
}

/*
    Returns \a strings compressed to the \a desiredSize.

    Compression works by replacing a section of two or more neighboring entries with a single "."
    character, with an aim to alternate between compressed and non-compressed sections of entries.
    The first and last entries are never compressed.

    For example, if the list {"a", "b", "c", "d", "e", "f", "g"} is compressed from its current
    size (7) to a desired size of 5, the b/c and e/f entries are compressed together, creating
    a resulting list of {"a", ".", "d", ".", "g"}.

    The compressedSections contains each set of compressed entries and the index of each
    related marker within the returned string list. For the above example, the map would contain:
        {
            { 1, {"b", "c" } },
            { 3, {"e", "f" } }
        }

    Depending on the size of the list, the size of the returned list may be one less than the
    desiredSize.
*/
QStringList SeasideStringListCompressor::compress(const QStringList &strings, int desiredSize, CompressedContent *compressedSections)
{
    if (strings.count() <= desiredSize || strings.count() < MinimumCompressionInputCount) {
        return strings;
    }

    QStringList ret = minimalCompress(strings, desiredSize, compressedSections);
    if (ret.isEmpty()) {
        ret = accordionCompress(strings, desiredSize, compressedSections);
    }
    return ret;
}

QStringList SeasideStringListCompressor::minimalCompress(const QStringList &strings, int desiredSize, CompressedContent *compressedSections)
{
    // First, determine whether we can use minimal compression (i.e. exactly 2 entries per compression marker).
    // To do this, determine the number of compression markers and uncompressed entries that we would need.
    // To support this minimal compression, we always need at least 1 more entry than compression markers.
    // e.g. ABCDEFGHIJ -> A.D.G.J etc, we will always have 1 more entry (4) than compression markers (3).
    int numCompressionMarkers = strings.size() - desiredSize;
    const int minNumUncompressedEntries = numCompressionMarkers + 1;
    bool canFitInDesiredSize = desiredSize >= (numCompressionMarkers + minNumUncompressedEntries);
    bool haveEnoughStrings = strings.size() >= (2*numCompressionMarkers + minNumUncompressedEntries);
    if (!canFitInDesiredSize || !haveEnoughStrings) {
        return QStringList();
    }

    // If we need 2 compression markers, then the compressed result will have 3 separate
    // uncompressed sections of contiguous entries.  e.g. ABCDEFGHIJ -> AB.EF.IJ
    // This is equal to the minimum number of uncompressed entries required,
    // but notionally is a distinct concept.
    const int numUncompressedSections = minNumUncompressedEntries;

    // Specify the size of our compressed sections.
    // We know that each compressed section will consist of exactly two entries.
    SectionsInfo compressedSectionsInfo;
    compressedSectionsInfo.minSize = 2;
    compressedSectionsInfo.maxSize = 2;
    compressedSectionsInfo.countMinSized = numCompressionMarkers;
    compressedSectionsInfo.countMaxSized = 0;

    // Calculate the sizes of our uncompressed sections.
    // We would LIKE to have equally sized uncompressed sections:
    // e.g. 18->15: ABCDEFGHIJKLMNOPQR -> ABC.FGH.KLM.PQR, each section is size 3.
    // but often we cannot:
    // e.g. 18->16: ABCDEFGHIJKLMNOPQR -> ABCDE.HIJK.NOPQR, some sections are 4, some are 5.
    SectionsInfo uncompressedSectionsInfo;
    uncompressedSectionsInfo.minSize = (desiredSize - numCompressionMarkers) / numUncompressedSections;
    uncompressedSectionsInfo.maxSize = (((desiredSize - numCompressionMarkers) % numUncompressedSections) == 0)
                                     ? uncompressedSectionsInfo.minSize
                                     : (uncompressedSectionsInfo.minSize+1);

    // Now calculate how many of each sized uncompressed section we need, in order to fulfill
    // the desired size, given that the number of compression markers is fixed.
    uncompressedSectionsInfo.countMinSized = numUncompressedSections;
    uncompressedSectionsInfo.countMaxSized = 0;
    while ((numCompressionMarkers
            + (uncompressedSectionsInfo.minSize * uncompressedSectionsInfo.countMinSized)
            + (uncompressedSectionsInfo.maxSize * uncompressedSectionsInfo.countMaxSized))
                    < desiredSize) {
        uncompressedSectionsInfo.countMaxSized++;
        uncompressedSectionsInfo.countMinSized--;
        if (uncompressedSectionsInfo.countMaxSized > numUncompressedSections) {
            // Something went wrong, we cannot satisfy the contraints.
            return QStringList();
        }
    }

    return performCompression(strings,
                              uncompressedSectionsInfo,
                              compressedSectionsInfo,
                              compressedSections);
}

QStringList SeasideStringListCompressor::accordionCompress(const QStringList &strings, int desiredSize, CompressedContent *compressedSections)
{
    // Accordion compression is required (every second entry in the returned list
    // will be a compression marker, accounting for a section of at least 2
    // but probably more actual entries).
    // We MUST return less than or equal to desiredSize or we will break the bounds.
    // If we have N compressed sections, then we will need N+1 uncompressed sections
    // each containing a single entry.
    // Thus, we will return an odd sized compressedList, to ensure we have alternating.
    // e.g. if desiredSize = 7, ABCDEFGHIJKLM -> A.E.I.M
    int possibleSize = ((desiredSize % 2) != 0) ? desiredSize : (desiredSize - 1);
    int numCompressionMarkers = possibleSize / 2;  // e.g. 3 out of 7 entries will be compressed section markers.
    int numUncompressedSections = numCompressionMarkers + 1; // e.g. the other 4 will be uncompressed sections.

    // Specify the size of our uncompressed sections.
    // We know that each uncompressed section will consist of a single entry.
    SectionsInfo uncompressedSectionsInfo;
    uncompressedSectionsInfo.minSize = 1;
    uncompressedSectionsInfo.maxSize = 1;
    uncompressedSectionsInfo.countMinSized = numUncompressedSections;
    uncompressedSectionsInfo.countMaxSized = 0;

    // Now determine the required sizes of our compressed sections, starting from
    // an initial assumption that each compressed section will contain either
    // two or three contiguous entries from the original list.
    // (Since it might not divide evenly, we may have some which are smaller and some larger.)
    // Also calculate how many compressed sections of each size we need.
    SectionsInfo compressedSectionsInfo;
    compressedSectionsInfo.minSize = 2;
    compressedSectionsInfo.maxSize = 3;

    bool foundSolution = false;
    while (!foundSolution) {
        // with the current compression section max+min sizes, try all combinations of section sizes.
        foundSolution = true;
        compressedSectionsInfo.countMinSized = numCompressionMarkers;
        compressedSectionsInfo.countMaxSized = 0;
        while ((numUncompressedSections
                + (compressedSectionsInfo.minSize * compressedSectionsInfo.countMinSized)
                + (compressedSectionsInfo.maxSize * compressedSectionsInfo.countMaxSized))
                        < strings.size()) {
            compressedSectionsInfo.countMinSized--;
            compressedSectionsInfo.countMaxSized++;
            if (compressedSectionsInfo.countMaxSized > numCompressionMarkers) {
                // no possible solutions with the current compression group sizes.
                foundSolution = false;
                break;
            }
        }

        if (!foundSolution) {
            compressedSectionsInfo.minSize++;
            compressedSectionsInfo.maxSize++;
        }

        if (compressedSectionsInfo.maxSize > strings.size()) {
            // This should never be reached.
            // A solution should always be possible.
            // But in case there's some edge case I haven't considered,
            // let's return an empty list instead of hanging.
            return QStringList();
        }
    }

    return performCompression(strings,
                              uncompressedSectionsInfo,
                              compressedSectionsInfo,
                              compressedSections);
}

QStringList SeasideStringListCompressor::performCompression(
        const QStringList &strings,
        SectionsInfo uncompressedSectionsInfo,
        SectionsInfo compressedSectionsInfo,
        CompressedContent *compressedSections)
{
    QStringList reversedStrings = strings;
    std::reverse(reversedStrings.begin(), reversedStrings.end());

    QList<QStringList> frontSections, backSections;
    QList<QStringList> frontCompressedSections, backCompressedSections;

    bool front = false;
    int frontPosition = 0, backPosition = 0;
    int remainingCompressedSections = compressedSectionsInfo.countMaxSized
                                    + compressedSectionsInfo.countMinSized;
    while (remainingCompressedSections > 0) {
        // Use sections of the appropriate size, based on the calculated
        // number of sections of each size we need to compress properly.
        const int uncompressedSectionSize = uncompressedSectionsInfo.countMaxSized-- > 0
                                          ? uncompressedSectionsInfo.maxSize
                                          : uncompressedSectionsInfo.minSize;
        const int compressedSectionSize = compressedSectionsInfo.countMinSized-- > 0
                                        ? compressedSectionsInfo.minSize
                                        : compressedSectionsInfo.maxSize;

        // Alternative between processing entries from the
        // front and back of the list, to ensure maximal symmetry.
        front = !front;

        // Deal with a single uncompressed section of entries
        if (front) {
            const QStringList uncompressedSection = strings.mid(
                    frontPosition, uncompressedSectionSize);
            frontSections.append(uncompressedSection);
            frontPosition += uncompressedSectionSize;
        } else {
            const QStringList uncompressedSection = reversedStrings.mid(
                    backPosition, uncompressedSectionSize);
            backSections.append(uncompressedSection);
            backPosition += uncompressedSectionSize;
        }

        // Deal with a single compressed section of entries
        if (front) {
            const QStringList compressedSection = strings.mid(
                    frontPosition, compressedSectionSize);
            frontSections.append(QStringList(QString(CompressionMarker)));
            frontCompressedSections.append(compressedSection);
            frontPosition += compressedSectionSize;
        } else {
            const QStringList compressedSection = reversedStrings.mid(
                    backPosition, compressedSectionSize);
            backSections.append(QStringList(QString(CompressionMarker)));
            backCompressedSections.append(compressedSection);
            backPosition += compressedSectionSize;
        }

        remainingCompressedSections--;
    }

    // Ensure that every entry in the original strings list is accounted for.
    // If not, add those entries (as a "middle" section might not have been included,
    // due to reaching the required compressed section count).
    const int sizeDelta = strings.size() - (frontPosition + backPosition);
    const QStringList middleSection = sizeDelta > 0
                                    ? strings.mid(frontPosition, sizeDelta)
                                    : QStringList();

    // Now build the complete compressed list we will return.
    QStringList orderedSections;
    for (int i = 0; i < frontSections.size(); ++i) {
        orderedSections.append(frontSections[i]);
    }
    orderedSections.append(middleSection);
    std::reverse(backSections.begin(), backSections.end());
    for (int i = 0; i < backSections.size(); ++i) {
        std::reverse(backSections[i].begin(), backSections[i].end());
        orderedSections.append(backSections[i]);
    }

    // And fill out the compressed sections out-parameter.
    std::reverse(backCompressedSections.begin(), backCompressedSections.end());
    for (int i = 0; i < backCompressedSections.size(); ++i) {
        std::reverse(backCompressedSections[i].begin(), backCompressedSections[i].end());
    }
    const QList<QStringList> allCompressedSections = frontCompressedSections + backCompressedSections;
    int compressedSectionsIndex = 0;
    for (int i = 0; i < orderedSections.size(); ++i) {
        if (orderedSections[i] == CompressionMarker) {
            compressedSections->insert(i, allCompressedSections[compressedSectionsIndex++]);
        }
    }

    return orderedSections;
}

bool SeasideStringListCompressor::isCompressionMarker(const QString &s)
{
    return s == CompressionMarker;
}

int SeasideStringListCompressor::minimumCompressionInputCount()
{
    return MinimumCompressionInputCount;
}

