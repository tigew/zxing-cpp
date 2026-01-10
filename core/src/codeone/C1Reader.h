/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing::CodeOne {

/**
 * Reader for Code One 2D matrix barcodes.
 * Supports versions A through H.
 */
class Reader : public ZXing::Reader
{
public:
	using ZXing::Reader::Reader;

	Barcode decode(const BinaryBitmap& image) const override;
};

} // namespace ZXing::CodeOne
