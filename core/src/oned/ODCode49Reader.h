/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class BitMatrix;

namespace OneD {

/**
 * Reader for Code 49 stacked barcodes.
 *
 * Code 49 was developed by David Allais at Intermec in 1987.
 * It was the first stacked barcode symbology and encodes the full
 * ASCII character set (128 characters).
 *
 * Structure:
 * - 2 to 8 rows stacked vertically
 * - Each row: Start (2 modules) + 4 symbol characters (64 modules) + Stop (4 modules) = 70 modules
 * - Each symbol character is 16 modules (4 bars + 4 spaces) encoding two code characters
 * - Rows separated by horizontal separator bars
 *
 * Encoding:
 * - 49 code character values (0-48)
 * - Two code characters per symbol character (49^2 = 2401 possible values)
 * - Even/odd parity patterns per row and position
 * - Row check: modulo 49 of sum of 7 codewords
 * - Symbol checks: X, Y (and Z for >6 rows) using weighted sums modulo 2401
 *
 * Capacity: Up to 81 alphanumeric or 49 bytes
 *
 * Standard: ANSI/AIM BC6-2000 (USS Code 49)
 */
class Code49Reader : public ZXing::Reader
{
public:
	explicit Code49Reader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
