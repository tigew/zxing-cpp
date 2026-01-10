/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Pharmazentralnummer (PZN) Reader
 *
 * PZN is a German pharmaceutical identification number encoded as a Code 39 variant.
 * It identifies medicines by name, dosage form, active ingredient strength, and pack size.
 *
 * Two versions exist:
 * - PZN7: 6 data digits + 1 check digit (phased out January 2020)
 * - PZN8: 7 data digits + 1 check digit (current standard since 2013)
 *
 * Format in barcode: *-XXXXXXXY* where:
 * - * = Code 39 start/stop character
 * - - = Minus sign (PZN identifier, reserved in ISO/IEC 15418)
 * - X = data digits
 * - Y = check digit (modulo 11)
 *
 * Check digit algorithm (Modulo 11):
 * - PZN7: weights 2,3,4,5,6,7 for 6 data digits
 * - PZN8: weights 1,2,3,4,5,6,7 for 7 data digits
 * - If check digit would be 10, the PZN is invalid
 *
 * Human readable: "PZN-XXXXXXXY" or "PZN XXXXXXXY"
 */
class PZNReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
