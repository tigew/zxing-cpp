/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Korea Post Barcode reader (Korean Postal Authority Code).
 *
 * This barcode encodes a 6-digit postal code plus a modulo-10 check digit.
 * It uses variable-width bars and spaces to encode each digit.
 */
class KoreaPostReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
