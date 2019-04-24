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
    character. Compression begins from the centre, then moves outwards towards the edges of the
    list, with an aim to alternate between compressed and non-compressed sets of entries. The
    first and last entries are never compressed.

    For example, if the list {"a", "b", "c", "d", "e", "f", "g"} is compressed from its count (7)
    to a target count of 5, the b/c and e/f entries are compressed together, creating a resulting
    list of {"a", ".", "d", ".", "g"}.

    Depending on the size of the list, the returned list may be one less than the target count.
*/
QStringList SeasideStringListCompressor::compress(const QStringList &strings, int compressTargetCount)
{
    if (strings.count() <= compressTargetCount || strings.count() < MinimumCompressionInputCount) {
        return strings;
    }

    QStringList result = strings;

    const bool listCountIsEven = result.count() % 2 == 0;
    const int centreIndex = result.count() / 2;
    int remaining = result.count();
    int firstBackwardsIndex = -1;
    int firstForwardsIndex = -1;
    int backwardsIndex = -1;
    int forwardsIndex = -1;

    if (listCountIsEven) {
        // If list is even-numbered, compress the two centre entries to begin with, before starting
        // the loop to compress more entries on either side.
        firstBackwardsIndex = centreIndex;
        firstForwardsIndex = centreIndex;
        backwardsIndex = compressAt(&result, &remaining, firstBackwardsIndex, ScanBackwards);

        // Next forward scan should ignore the compressed centre entries.
        forwardsIndex = nextCompressibleIndex(result, firstForwardsIndex, ScanForwards);
    } else {
        // If list is odd, keep the centre entry and start compressing entries on either side of it.
        firstBackwardsIndex = centreIndex - 1;
        firstForwardsIndex = centreIndex + 1;
        backwardsIndex = firstBackwardsIndex;
        forwardsIndex = firstForwardsIndex;
    }

    while (remaining > compressTargetCount) {
        backwardsIndex = compressAt(&result, &remaining, backwardsIndex, ScanBackwards);
        if (remaining <= compressTargetCount) {
            break;
        }
        if (backwardsIndex == -1) {
            backwardsIndex = nextCompressibleIndex(result, firstBackwardsIndex, ScanBackwards);
            if (backwardsIndex == -1) {
                break;
            }
        }

        forwardsIndex = compressAt(&result, &remaining, forwardsIndex, ScanForwards);
        if (remaining <= compressTargetCount) {
            break;
        }
        if (forwardsIndex == -1) {
            forwardsIndex = nextCompressibleIndex(result, firstForwardsIndex, ScanForwards);
            if (forwardsIndex == -1) {
                break;
            }
        }
    }

    QStringList compressedList;
    bool previousCompressed = false;
    for (const QString &sg : result) {
        if (isCompressionMarker(sg) && previousCompressed) {
            continue;
        }
        previousCompressed = isCompressionMarker(sg);
        compressedList.append(sg);
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

int SeasideStringListCompressor::findUncompressedNonEdgeString(const QStringList &strings, int fromIndex, ScanDirection direction)
{
    // Search all strings aside from first/last.
    while (fromIndex > 0 && fromIndex < strings.count() - 1) {
        if (!isCompressionMarker(strings.at(fromIndex))) {
            return fromIndex;
        }
        fromIndex += (direction == ScanForwards ? 1 : -1);
    }
    return -1;
}

/*
    Starting at \a fromIndex, search for the next index that can be compressed.

    The aim is to alternate between compressed and non-compressed entries, so this scans for the
    next non-compressed entry, skips over it, and returns the last compressed entry in the set after
    that.

    If the beginning or the end of the list is reached while scanning, the original fromIndex is
    returned.
*/
int SeasideStringListCompressor::nextCompressibleIndex(const QStringList &strings, int fromIndex, ScanDirection direction)
{
    const int singleScanDelta = (direction == ScanForwards ? 1 : -1);

    // Scan for the next non-compressed entry, then skip over it, and find the index after that
    // (which may or may not be compressed).
    int nextTarget = findUncompressedNonEdgeString(strings, fromIndex + singleScanDelta, direction);
    if (nextTarget < 0) {
        return fromIndex;
    }
    nextTarget += singleScanDelta;
    if (nextTarget <= 0 || nextTarget >= strings.count() - 1) {
        // Next target is first or last index, so stay on original index.
        return fromIndex;
    }

    // If next target is already compressed, find the last index in this compressed set, by scanning for
    // the next non-compressed entry then stepping back one index.
    if (isCompressionMarker(strings.at(nextTarget))) {
        int nextNonCompressed = findUncompressedNonEdgeString(strings, nextTarget, direction);
        if (nextNonCompressed < 0) {
            return fromIndex;
        }
        nextTarget = nextNonCompressed - singleScanDelta;
    }

    int neighborOfNextTarget = nextTarget + singleScanDelta;
    if (neighborOfNextTarget <= 0 || neighborOfNextTarget >= strings.count() - 1) {
        // Next target cannot be compressed as its neighbor cannot be compressed at the same time, so
        // just stay on the original index.
        return fromIndex;
    } else {
        return nextTarget;
    }
}

/*
    Compress the entry at \a index with its neighbor.
*/
int SeasideStringListCompressor::compressAt(QStringList *strings, int *remaining, int index, ScanDirection direction)
{
    const int scanDelta = (direction == ScanForwards ? 1 : -1);

    int neighborIndex = findUncompressedNonEdgeString(*strings, index + scanDelta, direction);
    if (neighborIndex < 0) {
        return -1;
    }

    QString current = strings->at(index);
    QString neighbor = strings->at(neighborIndex);

    current = CompressionMarker;
    strings->replace(index, current);

    neighbor = CompressionMarker;
    strings->replace(neighborIndex, neighbor);

    *(remaining) -= 1;

    // If the neighbor's neighbour is already compressed, that means two compressed sets are
    // now merged together, so update the remaining count.
    if (isCompressionMarker(strings->value(neighborIndex + scanDelta))) {
        *(remaining) -= 1;
    }

    return nextCompressibleIndex(*strings, neighborIndex, direction);
}
