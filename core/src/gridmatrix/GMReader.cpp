/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "GMReader.h"

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "GMDecoder.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

namespace ZXing::GridMatrix {

// Grid Matrix finder pattern
// The finder pattern is a 6x6 module pattern at corners
// Pattern: Dark-Light-Dark-Dark-Light-Dark (ratio 1:1:1:1:1:1)
static const int FINDER_PATTERN_SIZE = 6;

/**
 * Check if a position contains a Grid Matrix finder pattern.
 * Grid Matrix has finder patterns at corners with specific structure.
 */
static bool CheckFinderPattern(const BitMatrix& image, int x, int y, [[maybe_unused]] int moduleSize)
{
	// Check for Grid Matrix finder pattern structure
	// Alternating modules in specific pattern
	int darkCount = 0;
	int lightCount = 0;

	for (int dy = 0; dy < FINDER_PATTERN_SIZE; ++dy) {
		for (int dx = 0; dx < FINDER_PATTERN_SIZE; ++dx) {
			if (image.get(x + dx, y + dy)) {
				darkCount++;
			} else {
				lightCount++;
			}
		}
	}

	// Finder pattern should have specific dark/light ratio
	int total = darkCount + lightCount;
	if (total == 0)
		return false;

	float darkRatio = static_cast<float>(darkCount) / total;
	// Grid Matrix finder has approximately 50% dark modules
	return darkRatio >= 0.4f && darkRatio <= 0.6f;
}

/**
 * Validate Grid Matrix symbol dimensions.
 * Valid sizes: 18, 30, 42, 54, 66, 78, 90, 102, 114, 126, 138, 150, 162
 * Formula: 18 + 12*(version-1) for version 1-13
 */
static bool ValidateDimensions(int width, int height)
{
	// Must be square
	if (width != height)
		return false;

	// Check if size matches any valid version
	// Valid sizes are 18, 30, 42, ... 162 (18 + 12*n for n=0..12)
	if (width < 18 || width > 162)
		return false;

	// Size must be 18 + 12*n
	int adjusted = width - 18;
	if (adjusted < 0 || adjusted % 12 != 0)
		return false;

	return true;
}

/**
 * Estimate module size from the symbol.
 */
static int EstimateModuleSize(const BitMatrix& image, int left, int top, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
	// Grid Matrix uses 6-module macromodules
	// Try to find consistent module width by examining finder pattern

	int runLength = 0;
	int runsFound = 0;
	int totalRunLength = 0;
	bool lastBit = image.get(left, top);

	// Scan horizontally across the top
	for (int x = left; x < left + 36 && x < image.width(); ++x) {
		bool bit = image.get(x, top);
		if (bit == lastBit) {
			runLength++;
		} else {
			if (runsFound > 0) {
				totalRunLength += runLength;
			}
			runsFound++;
			runLength = 1;
			lastBit = bit;
		}
	}

	if (runsFound < 2)
		return 1;

	return std::max(1, totalRunLength / (runsFound - 1));
}

/**
 * Extract bits from a Grid Matrix symbol.
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

	// Estimate module size
	int moduleSize = EstimateModuleSize(image, left, top, width, height);
	if (moduleSize < 1)
		return {};

	// Check for finder patterns at corners
	if (!CheckFinderPattern(image, left, top, moduleSize))
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

	return Barcode(std::move(decRes), std::move(detRes), BarcodeFormat::GridMatrix);
}

} // namespace ZXing::GridMatrix
