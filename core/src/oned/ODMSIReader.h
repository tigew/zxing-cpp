/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * MSI (Modified Plessey) barcode reader.
 *
 * MSI is a numeric-only barcode symbology used primarily for inventory
 * control and warehousing. Each digit is encoded using 4 bits, where
 * each bit is represented by a bar/space pair:
 *   - 0 = narrow bar + wide space (1:2)
 *   - 1 = wide bar + narrow space (2:1)
 *
 * Start pattern: narrow bar, wide space (21)
 * Stop pattern: narrow bar, wide space, narrow bar (121)
 *
 * Check digit algorithms supported:
 *   - Mod 10 (Luhn-style)
 *   - Mod 11 (weighted sum)
 *   - Mod 10/10 (two Mod 10 checks)
 *   - Mod 11/10 (Mod 11 followed by Mod 10)
 */
class MSIReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
