/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * LOGMARS (Logistics Applications of Automated Marking and Reading Symbols) barcode reader.
 *
 * LOGMARS is a special application of Code 39 used by the U.S. Department of Defense
 * and is governed by Military Standard MIL-STD-1189B and MIL-STD-129.
 *
 * Characteristics:
 * - Based on Code 39 symbology (same encoding)
 * - Character set: 0-9, A-Z, space, and special chars (-, ., $, /, +, %)
 * - Each character: 5 bars + 4 spaces = 9 elements (3 wide, 6 narrow)
 * - Start/stop character: asterisk (*) - not included in data
 * - Check digit: Modulo 43 (MANDATORY, unlike optional in standard Code 39)
 * - Density: 3.0 to 9.4 characters per inch (CPI)
 *
 * LOGMARS vs Code 39:
 * - LOGMARS requires the Mod 43 check digit (Code 39 makes it optional)
 * - LOGMARS has specific density requirements for military applications
 * - LOGMARS uses symbology identifier ]L instead of ]A
 */
class LOGMARSReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
