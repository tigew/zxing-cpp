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
 * Royal Mail 4-State Mailmark barcode reader.
 *
 * Mailmark is a 4-state postal barcode used by Royal Mail in the UK, replacing RM4SCC
 * for machine-readable mail. It uses Reed-Solomon error correction and encodes mail
 * tracking information.
 *
 * Two variants:
 * - Barcode C: 22 characters, 66 bars (6 RS check symbols)
 * - Barcode L: 26 characters, 78 bars (7 RS check symbols)
 *
 * Data structure:
 * - Format (1 char): 0-4
 * - Version ID (1 char): 1-4
 * - Class (1 char): 0-9, A-E (15 values)
 * - Supply Chain ID: 2 digits (C) or 6 digits (L)
 * - Item ID (8 digits)
 * - Destination Postcode + DPS (9 chars)
 *
 * Encoding:
 * - Each 3-bar group represents a 6-bit extender value
 * - Bar states: F (Full), A (Ascender), D (Descender), T (Tracker)
 * - Reed-Solomon GF(64) with primitive polynomial x^6 + x^5 + x^2 + 1
 */
class MailmarkReader : public ZXing::Reader
{
public:
	explicit MailmarkReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
