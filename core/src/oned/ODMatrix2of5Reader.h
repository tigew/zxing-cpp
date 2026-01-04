/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * @brief Reader for Matrix 2 of 5 barcode format.
 *
 * Matrix 2 of 5 (also known as Code 2 of 5 Matrix or Standard 2 of 5) is a discrete,
 * numeric-only barcode symbology developed by Nieaf Co. in the Netherlands in the 1970s.
 *
 * Key characteristics:
 * - Encodes digits 0-9 only
 * - Each digit encoded with 3 bars and 3 spaces (6 elements total)
 * - 2 of 5 elements (bars and spaces) are wide, 3 are narrow
 * - Discrete symbology (characters separated by inter-character gap)
 * - Optional mod 10/3 check digit
 *
 * Used for warehouse sorting, photo finishing, and airline ticket marking.
 */
class Matrix2of5Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
