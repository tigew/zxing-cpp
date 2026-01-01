/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Telepen barcode reader.
 *
 * Telepen was developed in 1972 by SB Electronic Systems Ltd in the UK.
 * It is a continuous, variable-length symbology that can encode all 128
 * ASCII characters without using shift characters.
 *
 * Characteristics:
 * - Full ASCII (0-127) character set
 * - Each character: 16 modules wide (4-8 bars/spaces)
 * - Elements: narrow (1 module) or wide (3 modules), 3:1 ratio
 * - Start character: "_" (underscore, ASCII 95)
 * - Stop character: "z" (lowercase, ASCII 122)
 * - Check digit: modulo 127 (hidden, included in barcode)
 * - Even parity on each character byte
 *
 * Two modes:
 * - Full ASCII: encodes all 128 ASCII characters
 * - Numeric: double-density encoding for digits (pairs of digits)
 *
 * Check digit algorithm:
 * - Sum all ASCII values of data characters
 * - Check digit = (127 - (sum % 127)) % 127
 */
class TelepenReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
