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

#include "seasidestringlistcompressor.h"

#include <QDebug>

namespace {

const QChar CompressionMarker = QChar('.');
const int MinimumCompressionInputCount = 5;

}

SeasideStringListCompressor::SeasideStringListCompressor()
{
}

SeasideStringListCompressor::~SeasideStringListCompressor()
{
}

/*
    Returns \a strings compressed to the \a compressTargetCount.

    Compression works by grouping two or more neighboring entries together into a single "."
    character, with an aim to alternate between compressed and non-compressed sets of entries. The
    first and last entries are never compressed.

    For example, if the list {"a", "b", "c", "d", "e", "f", "g"} is compressed from its count (7)
    to a target count of 5, the b/c and e/f entries are compressed together, creating a resulting
    list of {"a", ".", "d", ".", "g"}.

    The compressedContent contains each set of compressed entries and the index of each
    related marker within the returned string list. For the above example, the map would contain:
        {
            { 1, {"b", "c" } },
            { 3, {"e", "f" } }
        }

    Depending on the size of the list, the returned list may be one less than the target count.
*/
QStringList SeasideStringListCompressor::compress(const QStringList &strings, int compressTargetCount, CompressedContent *compressedContent)
{
    if (strings.count() <= compressTargetCount || strings.count() < MinimumCompressionInputCount) {
        return strings;
    }

    if (compressTargetCount % 2 != 0) {
        compressTargetCount++;
    }

    // Number of compression markers to be added. -1 to drop the first entry (since that
    // cannot be compressed) and /2 as markers are placed at every second index.
    const int markerCount = (compressTargetCount - 1) / 2;

    // Max number of list entries to be represented by each compression marker (minimum 2).
    int maxEntriesPerMarker = 2;
    while (((strings.count() - (maxEntriesPerMarker * markerCount)) + markerCount) >= compressTargetCount) {
        maxEntriesPerMarker++;
    }

    QStringList compressedList;
    QStringList currentMarkerGroup;
    int nextUncompressedIndex = 0;
    bool firstCompression = true;
    int entriesPerMarker = initialEntriesPerMarker(strings, markerCount, maxEntriesPerMarker, compressTargetCount);

    // Add alternating sets of list entries and compression markers. For each compression marker,
    // update compressedContent with the marker index and the strings represented by that marker.
    for (int i = 0; i < strings.count(); ++i) {
        if (i == nextUncompressedIndex) {
            if (!currentMarkerGroup.isEmpty()) {
                compressedContent->insert(compressedList.count() - 1, currentMarkerGroup);
                currentMarkerGroup.clear();
            }
            // Append the current string entry.
            compressedList.append(strings[i]);
            nextUncompressedIndex = i + entriesPerMarker + 1;

            // Append a compression marker if the next index is part of a compressed group and
            // this is not the last item in the list.
            if (nextUncompressedIndex > i+1 && i < strings.count()-1) {
                compressedList.append(CompressionMarker);
            }

            // Check whether this is the last entry in the list.
            if (nextUncompressedIndex >= strings.count() - 1) {
                // The last group extends to the second-last entry in the list.
                QStringList lastMarkerGroup = strings.mid(i + 1, strings.count() - i - 2);
                if (!lastMarkerGroup.isEmpty()) {
                    compressedContent->insert(compressedList.count() - 1, lastMarkerGroup);
                }
                compressedList.append(strings.last());
                break;
            }
        } else {
            currentMarkerGroup.append(strings[i]);
        }

        if (firstCompression) {
            firstCompression = false;
            entriesPerMarker = maxEntriesPerMarker;
        }
    }

    return compressedList;
}

bool SeasideStringListCompressor::isCompressionMarker(const QString &s)
{
    return s == CompressionMarker;
}

int SeasideStringListCompressor::minimumCompressionInputCount()
{
    return MinimumCompressionInputCount;
}

/*
    Returns an optimal size for the first compression marker group.

    This balances the first and last marker groups so that they have the same size (+/-1).
*/
int SeasideStringListCompressor::initialEntriesPerMarker(const QStringList &strings, int markerCount, int maxEntriesPerMarker, int compressTargetCount)
{
    const int maxEntriesRemoved = markerCount * maxEntriesPerMarker;
    const int actualEntriesRemoved = ((strings.count() + markerCount) - compressTargetCount) + 1;

    const int lastMarkerGroupLength = maxEntriesPerMarker - (maxEntriesRemoved % actualEntriesRemoved);
    const int firstAndLastMarkerGroupsTotal = maxEntriesPerMarker + lastMarkerGroupLength;

    return firstAndLastMarkerGroupsTotal / 2;
}
