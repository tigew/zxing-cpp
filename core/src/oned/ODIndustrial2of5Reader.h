/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * @brief Reader for Industrial 2 of 5 barcode format.
 *
 * Industrial 2 of 5 (also known as Standard 2 of 5, Code 2 of 5 Industrial) is a discrete,
 * numeric-only barcode symbology invented in 1971 by Identicon Corp and Computer Identics Corp.
 *
 * Key characteristics:
 * - Encodes digits 0-9 only
 * - Each digit encoded with 5 bars and 5 spaces (10 elements total)
 * - Only bars encode data (2 wide, 3 narrow), spaces are fixed-width (narrow)
 * - Discrete symbology (characters separated by inter-character gap)
 * - Optional modulo 10 check digit
 *
 * Used for airline tickets, warehouse sorting, and industrial inventory tracking.
 */
class Industrial2of5Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
