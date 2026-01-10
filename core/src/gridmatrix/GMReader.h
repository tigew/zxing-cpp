/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing::GridMatrix {

/**
 * Reader for Grid Matrix 2D barcodes.
 * Grid Matrix is a Chinese national standard (GB/T 21049-2007) 2D symbology
 * optimized for encoding Chinese characters (GB 2312).
 *
 * Features:
 * - 13 versions (V1-V13): 18x18 to 162x162 modules
 * - Macromodule structure: 6x6 module blocks
 * - 5 error correction levels (L1-L5)
 * - Encoding modes: Numeric, Uppercase, Mixed, Chinese (GB 2312), Binary
 */
class Reader : public ZXing::Reader
{
public:
	using ZXing::Reader::Reader;

	Barcode decode(const BinaryBitmap& image) const override;
};

} // namespace ZXing::GridMatrix
