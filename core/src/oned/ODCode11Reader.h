/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Code 11 barcode reader.
 *
 * Code 11 (also known as USD-8) is a barcode symbology developed by Intermec
 * in 1977, primarily used in telecommunications for labeling components.
 *
 * Characteristics:
 * - Character set: 0-9 and dash (-)
 * - Each character: 3 bars + 2 spaces (5 elements)
 * - Start/stop character: special pattern (printed as "*")
 * - Check digits: C (modulo 11) and optionally K (modulo 11)
 *   - Data length <= 10: single C check digit
 *   - Data length > 10: both C and K check digits
 *
 * Check digit algorithm:
 * - C: Sum of (value * weight) mod 11, weights 1-10 cycling from right
 * - K: Sum of (value * weight) mod 11, weights 1-9 cycling from right (includes C)
 * - Dash (-) has value 10 in calculations
 */
class Code11Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
