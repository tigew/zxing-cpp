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
 * @brief Reader for Japan Post 4-State Customer Code (Kasutama Barcode).
 *
 * Japan Post barcode is used by Japan Post for postal code and address
 * encoding on mail items. It uses 4-state bars for data encoding.
 *
 * Technical specifications:
 * - 4-state bars: Full (4), Ascender (2), Descender (3), Tracker (1)
 * - Character set: 0-9, A-Z, hyphen (-)
 * - Digits/hyphen: 3 bars per character
 * - Letters: 6 bars (control code + digit)
 * - Fixed structure: Start(2) + PostalCode(21) + Address(39) + CheckDigit(3) + Stop(2) = 67 bars
 * - Check digit: modulo 19 checksum
 */
class JapanPostReader : public ZXing::Reader
{
public:
	explicit JapanPostReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
