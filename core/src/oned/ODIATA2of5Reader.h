/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * @brief Reader for IATA 2 of 5 barcode format.
 *
 * IATA 2 of 5 (also known as Airline 2 of 5, Computer Identics 2 of 5) is a discrete,
 * numeric-only barcode symbology used by the International Air Transport Association.
 *
 * Key characteristics:
 * - Encodes digits 0-9 only
 * - Each digit encoded with 5 bars and 5 spaces (10 elements total)
 * - Only bars encode data (2 wide, 3 narrow), spaces are fixed-width (narrow)
 * - Discrete symbology (characters separated by inter-character gap)
 * - Optional modulo 10 check digit
 * - Shorter start/stop patterns than Industrial 2 of 5
 *
 * Used for airline cargo management, baggage handling, and air transport logistics.
 */
class IATA2of5Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
