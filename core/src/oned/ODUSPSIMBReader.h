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
 * USPS Intelligent Mail Barcode (IMb) reader.
 *
 * Also known as OneCode, 4-State Customer Barcode (4CB), or USPS4CB.
 * A 65-bar 4-state barcode used by the United States Postal Service.
 *
 * Structure:
 * - 65 bars with 4 states: Full (F), Ascender (A), Descender (D), Tracker (T)
 * - 130 total bits (each bar has ascender bit + descender bit)
 * - 10 codewords of 13 bits each
 * - 11-bit CRC for error detection
 *
 * Data fields:
 * - Barcode Identifier (2 digits): 0-4
 * - Service Type Identifier (3 digits): 000-999
 * - Mailer ID (6 or 9 digits)
 * - Serial Number (9 or 6 digits, together with Mailer ID = 15 digits)
 * - Routing Code (0, 5, 9, or 11 digits ZIP code)
 *
 * Reference: USPS-B-3200 Intelligent Mail Barcode 4-State Specification
 */
class USPSIMBReader : public ZXing::Reader
{
public:
	explicit USPSIMBReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
