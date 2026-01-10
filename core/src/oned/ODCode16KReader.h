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
 * Reader for Code 16K stacked barcodes.
 *
 * Code 16K was developed by Ted Williams at Laserlight Systems in 1992.
 * It is a stacked symbology based on Code 128, using 2-16 rows to encode data.
 *
 * Structure:
 * - Each row: Start (4 bars) + Separator (1 bar) + 5 codewords (6 bars each) + Stop (4 bars)
 * - Total row width: 70 modules (plus quiet zones)
 * - Uses 8 different start/stop patterns indexed by row number
 * - Row indicator in first codeword: (7 * (rows - 2)) + mode
 *
 * Encoding:
 * - Uses Code 128 character sets A, B, and C
 * - 107 distinct patterns (same as Code 128)
 * - Two modulo-107 check digits
 * - Maximum capacity: 77 ASCII or 154 numeric characters
 *
 * Applications: Electronics and medical industries (US, France)
 *
 * This reader requires 2D access to detect and decode multiple rows.
 */
class Code16KReader : public ZXing::Reader
{
public:
	explicit Code16KReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
