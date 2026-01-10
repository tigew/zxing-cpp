/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "C1Reader.h"

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "C1Decoder.h"
#include "C1Version.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

namespace ZXing::CodeOne {

/**
 * Detect Code One finder pattern.
 * Code One has a distinctive horizontal bar pattern on the left edge.
 * The finder pattern consists of alternating black and white horizontal bars.
 */
static bool HasFinderPattern(const BitMatrix& image, int left, int top, [[maybe_unused]] int width, int height)
{
	// Check leftmost column for alternating pattern
	int transitions = 0;
	bool lastBit = image.get(left, top);

	for (int y = top + 1; y < top + height; ++y) {
		bool bit = image.get(left, y);
		if (bit != lastBit) {
			transitions++;
			lastBit = bit;
		}
	}

	// Code One should have multiple horizontal transitions in the finder
	// Versions A-H have at least 4 horizontal bars
	return transitions >= 3;
}

/**
 * Validate Code One dimensions against known versions.
 */
static const Version* ValidateDimensions(int width, int height)
{
	// Allow some tolerance for quiet zone variations
	for (const auto& v : VERSIONS) {
		if (std::abs(width - v.width) <= 2 && std::abs(height - v.height) <= 2) {
			return &v;
		}
	}
	return nullptr;
}

/**
 * Extract pure bits from a Code One symbol.
 * Assumes the image contains only the barcode with minimal quiet zone.
 */
static DetectorResult ExtractPureBits(const BitMatrix& image)
{
	int left, top, width, height;

	// Find bounding box of the symbol
	// Code One minimum size is Version A: 16x18
	if (!image.findBoundingBox(left, top, width, height, 16))
		return {};

	// Validate dimensions match a known version
	const Version* version = ValidateDimensions(width, height);
	if (!version)
		return {};

	// Check for finder pattern
	if (!HasFinderPattern(image, left, top, width, height))
		return {};

	// Sample the bits
	BitMatrix bits(version->width, version->height);

	for (int y = 0; y < version->height; ++y) {
		int iy = top + (y * height + height / 2) / version->height;
		for (int x = 0; x < version->width; ++x) {
			int ix = left + (x * width + width / 2) / version->width;
			if (image.get(ix, iy)) {
				bits.set(x, y);
			}
		}
	}

	return {std::move(bits), Rectangle<PointI>(left, top, width, height)};
}

Barcode Reader::decode(const BinaryBitmap& image) const
{
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

	// For now, only handle pure barcodes
	// A full implementation would include a proper detector
	auto detRes = ExtractPureBits(*binImg);
	if (!detRes.isValid())
		return {};

	auto decRes = Decode(detRes.bits());
	if (!decRes.isValid(_opts.returnErrors()))
		return {};

	return Barcode(std::move(decRes), std::move(detRes), BarcodeFormat::CodeOne);
}

} // namespace ZXing::CodeOne
