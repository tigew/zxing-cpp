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
 * Reader for Codablock F stacked barcodes.
 *
 * Codablock F is a stacked symbology based on Code 128, developed by Identcode
 * Systeme GmbH in Germany in 1989. It consists of 2-44 rows of Code 128 symbols
 * separated by horizontal separator bars.
 *
 * Structure:
 * - Each row contains: Start Code A + Row Indicator + Data + Row Checksum + Stop
 * - First row: Row indicator = (rows - 2), subsequent rows: row number + 42
 * - Last row includes K1/K2 check characters (modulo 86 algorithm)
 * - Each row has a modulo 103 checksum
 *
 * Encoding:
 * - Uses Code 128 character sets A, B, and C
 * - Can encode full ISO 8859-1 (extended ASCII) using FNC4
 * - Maximum data capacity: 2725 characters
 *
 * This reader requires 2D access to detect and decode multiple rows.
 */
class CodablockFReader : public ZXing::Reader
{
public:
	explicit CodablockFReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
