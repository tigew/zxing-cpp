/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Datalogic 2 of 5 (Code 2 of 5 Data Logic, China Post) barcode reader.
 *
 * Developed by Datalogic in 1979. Used by Chinese Postal Service for mail sorting.
 * Numeric-only symbology (0-9) using Matrix encoding with IATA start/stop patterns.
 *
 * Structure: 6 elements per digit (bar-space-bar-space-bar-space)
 * Encoding: Both bars and spaces encode data (like Matrix 2 of 5)
 * Start pattern: 1,1,1,1 (narrow-bar, narrow-space, narrow-bar, narrow-space)
 * Stop pattern: 3,1,1 (wide-bar, narrow-space, narrow-bar)
 */
class Datalogic2of5Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
