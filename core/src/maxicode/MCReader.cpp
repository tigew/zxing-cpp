/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2026 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "MCReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "MCBitMatrixParser.h"
#include "MCDecoder.h"
#include "MCDetector.h"

namespace ZXing::MaxiCode {

Barcode Reader::decode(const BinaryBitmap& image) const
{
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

	// Use the new detector which handles rotation, skew, and perspective
	auto detRes = Detect(*binImg, _opts.isPure(), _opts.tryHarder());
	if (!detRes.isValid())
		return {};

	DecoderResult decRes = Decode(detRes.bits());
	if (!decRes.isValid())
		return {};

	return Barcode(std::move(decRes), std::move(detRes), BarcodeFormat::MaxiCode);
}

} // namespace ZXing::MaxiCode
