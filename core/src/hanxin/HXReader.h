/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing::HanXin {

/**
 * Reader for Han Xin Code 2D barcodes.
 * Han Xin Code is a Chinese national standard (GB/T 21049, ISO/IEC 20830)
 * 2D symbology optimized for encoding Chinese characters.
 *
 * Features:
 * - 84 versions: 23x23 to 189x189 modules (size = version*2 + 21)
 * - 4 error correction levels (L1-L4)
 * - 7 encoding modes: Numeric, Text, Binary, Region1/2 Chinese, Double-byte, Four-byte
 * - GB 18030 character set support
 * - Reed-Solomon error correction over GF(256)
 */
class Reader : public ZXing::Reader
{
public:
	using ZXing::Reader::Reader;

	Barcode decode(const BinaryBitmap& image) const override;
};

} // namespace ZXing::HanXin
