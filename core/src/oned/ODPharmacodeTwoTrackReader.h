/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class BinaryBitmap;
class BitMatrix;

namespace OneD {

/**
 * Pharmacode Two-Track (2D Pharmacode) Reader
 *
 * Pharmacode Two-Track is a 3-state barcode developed by Laetus GmbH
 * for pharmaceutical packaging control. It encodes integers from 4 to 64,570,080.
 *
 * Key characteristics:
 * - Uses 3 bar types: Full, Upper half (ascender), Lower half (descender)
 * - Bijective base-3 encoding (digits 1, 2, 3 instead of 0, 1, 2)
 * - Read from right to left
 * - No start/stop patterns, no check digit
 * - 2-16 bars
 *
 * Bar values (position n from right, starting at 0):
 * - Full bar: contributes 1 × 3^n
 * - Lower half (descender): contributes 2 × 3^n
 * - Upper half (ascender): contributes 3 × 3^n
 */
class PharmacodeTwoTrackReader : public ZXing::Reader
{
public:
	explicit PharmacodeTwoTrackReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
