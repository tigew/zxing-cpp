/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Pharmacode (Laetus) Reader
 *
 * Pharmacode is a binary barcode used in pharmaceutical packaging for control systems.
 * Developed by Laetus GmbH (Germany).
 *
 * Key characteristics:
 * - Encodes integers from 3 to 131,070
 * - Uses two bar widths: narrow and wide (3:1 ratio)
 * - Read from right to left (unidirectional)
 * - No start/stop characters
 * - No check digit
 * - 2 to 16 bars
 *
 * Decoding algorithm:
 * - Narrow bar at position n (from right, starting at 0): contributes 2^n
 * - Wide bar at position n: contributes 2 * 2^n = 2^(n+1)
 */
class PharmacodeReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
