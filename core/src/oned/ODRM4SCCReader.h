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
 * Royal Mail 4-State Customer Code (RM4SCC) reader.
 *
 * RM4SCC is a 4-state postal barcode used by Royal Mail in the UK.
 * Structure: Start bar + Data + Checksum + Stop bar
 * - Start bar: Single Ascender bar
 * - Stop bar: Single Full bar
 * - Data: 5-7 character postcode + 2-character Delivery Point Suffix (DPS)
 * - Checksum: Calculated from data using modulo 6 algorithm
 *
 * Character set: 0-9, A-Z (36 characters)
 * Each character is encoded with 4 bars using the Royal Table encoding.
 */
class RM4SCCReader : public ZXing::Reader
{
public:
	explicit RM4SCCReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
