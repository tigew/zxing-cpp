/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Reader for Australia Post 4-State barcodes.
 *
 * Supports all four variants:
 * - Standard Customer Barcode (FCC 11) - 37 bars
 * - Reply Paid Barcode (FCC 45) - 37 bars
 * - Routing Barcode (FCC 87) - 37 bars
 * - Redirection Barcode (FCC 92) - 37 bars
 * - Customer Barcode 2 (FCC 59) - 52 bars
 * - Customer Barcode 3 (FCC 62) - 67 bars
 *
 * The barcode uses 4 bar states:
 * - F (Full): Full height bar (ascender + descender)
 * - A (Ascender): Top half bar only
 * - D (Descender): Bottom half bar only
 * - T (Tracker): Short middle bar only
 */
class AustraliaPostReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>& state) const override;
};

} // namespace ZXing::OneD
