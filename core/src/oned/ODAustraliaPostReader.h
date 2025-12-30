/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class BitMatrix;

namespace OneD {

/**
 * Reader for Australia Post 4-State Customer Barcodes.
 *
 * This reader fully implements decoding of all Australia Post 4-state postal barcode variants:
 * - Standard Customer Barcode (FCC 11) - 37 bars
 * - Reply Paid Barcode (FCC 45) - 37 bars
 * - Routing Barcode (FCC 87) - 37 bars
 * - Redirection Barcode (FCC 92) - 37 bars
 * - Customer Barcode 2 (FCC 59) - 52 bars
 * - Customer Barcode 3 (FCC 62) - 67 bars
 *
 * 4-State Bar Encoding:
 * - F (Full): Full height bar (value 3) - extends from baseline to topline
 * - A (Ascender): Top half bar (value 2) - extends from center to topline
 * - D (Descender): Bottom half bar (value 1) - extends from baseline to center
 * - T (Tracker): Short center bar (value 0) - only in middle section
 *
 * The barcode uses Reed-Solomon error correction with GF(64) and can correct
 * up to 2 symbol errors or 4 erasures.
 */
class AustraliaPostReader : public ZXing::Reader
{
public:
	explicit AustraliaPostReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
