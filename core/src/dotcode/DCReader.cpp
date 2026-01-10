/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "DCReader.h"

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "DCDecoder.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

namespace ZXing::DotCode {

/**
 * Check if the pattern matches DotCode checkerboard dot arrangement.
 * DotCode dots follow: even columns have dots on odd rows, odd columns have dots on even rows.
 */
static bool HasDotCodePattern(const BitMatrix& image, int left, int top, int width, int height)
{
	int dotCount = 0;
	int expectedDots = 0;

	// Check a sample of positions for dot pattern
	for (int y = top; y < top + height; ++y) {
		for (int x = left; x < left + width; ++x) {
			// In DotCode, dots can only appear at checkerboard positions
			if ((x + y) % 2 == 0) {
				expectedDots++;
				if (image.get(x, y)) {
					dotCount++;
				}
			} else {
				// Non-dot positions should typically be empty
				// Allow some tolerance for noise
			}
		}
	}

	// DotCode symbols typically have 40-60% dot density at valid positions
	if (expectedDots == 0)
		return false;

	float density = static_cast<float>(dotCount) / expectedDots;
	return density >= 0.3f && density <= 0.8f;
}

/**
 * Validate DotCode dimensions.
 * DotCode has minimum size 5x5 and no maximum.
 */
static bool ValidateDimensions(int width, int height)
{
	// Minimum size is 5x5
	if (width < 5 || height < 5)
		return false;

	// Width and height should both be odd for standard DotCode
	// But allow even dimensions for flexibility
	return true;
}

/**
 * Extract bits from a DotCode symbol.
 * Assumes the image contains only the barcode.
 */
static DetectorResult ExtractPureBits(const BitMatrix& image)
{
	int left, top, width, height;

	// Find bounding box of the symbol
	if (!image.findBoundingBox(left, top, width, height, 5))
		return {};

	// Validate dimensions
	if (!ValidateDimensions(width, height))
		return {};

	// Check for DotCode pattern
	if (!HasDotCodePattern(image, left, top, width, height))
		return {};

	// Sample the bits
	BitMatrix bits(width, height);

	for (int y = 0; y < height; ++y) {
		int iy = top + y;
		for (int x = 0; x < width; ++x) {
			int ix = left + x;
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

	return Barcode(std::move(decRes), std::move(detRes), BarcodeFormat::DotCode);
}

} // namespace ZXing::DotCode
