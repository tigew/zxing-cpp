/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing::DotCode {

/**
 * Reader for DotCode 2D dot matrix barcodes.
 * DotCode is designed for high-speed industrial printing using dots.
 */
class Reader : public ZXing::Reader
{
public:
	using ZXing::Reader::Reader;

	Barcode decode(const BinaryBitmap& image) const override;
};

} // namespace ZXing::DotCode
