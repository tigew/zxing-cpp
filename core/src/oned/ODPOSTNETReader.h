/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class BinaryBitmap;
class BitMatrix;

namespace OneD {

/**
 * POSTNET and PLANET barcode reader.
 *
 * POSTNET (Postal Numeric Encoding Technique) and PLANET (Postal Alpha Numeric
 * Encoding Technique) are height-modulated barcodes used by the United States
 * Postal Service for mail sorting and tracking.
 *
 * Both formats use 2-state bars (tall and short):
 * - POSTNET: Each digit encoded with 2 tall + 3 short bars
 * - PLANET: Each digit encoded with 3 tall + 2 short bars (inverse of POSTNET)
 *
 * Structure:
 * - Start frame bar (tall)
 * - Data digits (5-digit ZIP, ZIP+4, or 11-digit DPBC for POSTNET; 12 or 14 for PLANET)
 * - Check digit (mod 10)
 * - End frame bar (tall)
 *
 * Digit encoding uses weights 7-4-2-1-0 (left to right):
 * - Digit 0 special: encoded as 11000 (7+4=11)
 * - Digits 1-9: encoded as sum of weights
 *
 * Bar lengths (POSTNET):
 * - 32 bars: 5-digit ZIP (5 data + 1 check = 6 digits × 5 + 2 frame = 32)
 * - 52 bars: ZIP+4 (9 data + 1 check = 10 digits × 5 + 2 frame = 52)
 * - 62 bars: 11-digit DPBC (11 data + 1 check = 12 digits × 5 + 2 frame = 62)
 *
 * Bar lengths (PLANET):
 * - 62 bars: 12 digits (12 digits × 5 + 2 frame = 62)
 * - 72 bars: 14 digits (14 digits × 5 + 2 frame = 72)
 */
class POSTNETReader : public ZXing::Reader
{
public:
	explicit POSTNETReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
