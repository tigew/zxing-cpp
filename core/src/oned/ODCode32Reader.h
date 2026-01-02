/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Code 32 (Italian Pharmacode) Reader
 *
 * Code 32 is a variant of Code 39 used exclusively in the Italian pharmaceutical industry.
 * It encodes 9 digits (8 data digits + 1 mod-10 check digit) as 6 base-32 characters
 * using the Code 39 character encodings.
 *
 * The base-32 alphabet consists of: 0-9, B, C, D, F, G, H, J, K, L, M, N, P, Q, R, S, T, U, V, W, X, Y, Z
 * (excludes vowels A, E, I, O to avoid confusion)
 *
 * The human-readable form is prefixed with 'A' (not encoded in the barcode).
 */
class Code32Reader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
