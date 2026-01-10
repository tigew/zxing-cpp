/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Channel Code Reader
 *
 * Channel Code is a compact linear barcode symbology designed to encode numeric values
 * in the shortest possible length. It was invented by Ted Williams and Andy Longacre in 1992.
 * Defined in ANSI/AIM BC12 - USS Channel Code.
 *
 * Key characteristics:
 * - Encodes numeric values from 0 to 7,742,862
 * - Six channels (3-8) with different capacities:
 *   - Channel 3: 0-26 (2 digits)
 *   - Channel 4: 0-292 (3 digits)
 *   - Channel 5: 0-3493 (4 digits)
 *   - Channel 6: 0-44072 (5 digits)
 *   - Channel 7: 0-576688 (6 digits)
 *   - Channel 8: 0-7742862 (7 digits)
 * - Self-checking, bidirectional
 * - Finder pattern of 9 consecutive bar modules
 * - No explicit check digit (self-correcting)
 *
 * Symbol structure:
 * - Finder pattern: 9 modules of bar
 * - Data: n bars and n-1 spaces for channel n
 * - Each element width: 1-9 modules
 */
class ChannelCodeReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
