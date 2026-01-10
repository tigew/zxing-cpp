/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "HXReader.h"

#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "HXDecoder.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

namespace ZXing::HanXin {

// Han Xin finder pattern structure
// Finder patterns are located at the four corners of the symbol
// Each finder pattern is 7x7 modules with a specific pattern
static const int FINDER_PATTERN_SIZE = 7;

/**
 * Check if a position contains a Han Xin finder pattern.
 * Han Xin has distinctive corner finder patterns.
 */
static bool CheckFinderPattern(const BitMatrix& image, int x, int y, [[maybe_unused]] int moduleSize)
{
	// Check bounds
	if (x + FINDER_PATTERN_SIZE > image.width() || y + FINDER_PATTERN_SIZE > image.height())
		return false;

	int darkCount = 0;
	int lightCount = 0;

	// Count dark and light modules in the finder region
	for (int dy = 0; dy < FINDER_PATTERN_SIZE; ++dy) {
		for (int dx = 0; dx < FINDER_PATTERN_SIZE; ++dx) {
			if (image.get(x + dx, y + dy)) {
				darkCount++;
			} else {
				lightCount++;
			}
		}
	}

	// Han Xin finder patterns have approximately 24 dark modules out of 49
	int total = darkCount + lightCount;
	if (total == 0)
		return false;

	float darkRatio = static_cast<float>(darkCount) / total;
	return darkRatio >= 0.35f && darkRatio <= 0.65f;
}

/**
 * Validate Han Xin symbol dimensions.
 * Valid sizes: 23, 25, 27, ... 189 (23 + 2*n for n=0..83)
 * Formula: size = version * 2 + 21 for version 1-84
 */
static bool ValidateDimensions(int width, int height)
{
	// Must be square
	if (width != height)
		return false;

	// Check size range
	if (width < 23 || width > 189)
		return false;

	// Size must be 21 + 2*n (odd numbers from 23 to 189)
	if ((width - 21) % 2 != 0)
		return false;

	return true;
}

/**
 * Estimate module size from the symbol.
 */
static int EstimateModuleSize(const BitMatrix& image, int left, int top, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
	// Han Xin finder patterns are 7 modules
	// Try to find consistent module width by examining finder pattern

	int runLength = 0;
	int runsFound = 0;
	int totalRunLength = 0;
	bool lastBit = image.get(left, top);

	// Scan horizontally across the finder pattern area
	for (int x = left; x < left + 21 && x < image.width(); ++x) {
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
 * Detect Han Xin finder patterns at corners.
 */
static bool DetectFinderPatterns(const BitMatrix& image, int left, int top, int width, int height, int moduleSize)
{
	// Check all four corners for finder patterns
	bool topLeft = CheckFinderPattern(image, left, top, moduleSize);
	bool topRight = CheckFinderPattern(image, left + width - FINDER_PATTERN_SIZE, top, moduleSize);
	bool bottomLeft = CheckFinderPattern(image, left, top + height - FINDER_PATTERN_SIZE, moduleSize);
	bool bottomRight = CheckFinderPattern(image, left + width - FINDER_PATTERN_SIZE, top + height - FINDER_PATTERN_SIZE, moduleSize);

	// At least 3 finder patterns should be detected
	int foundCount = (topLeft ? 1 : 0) + (topRight ? 1 : 0) + (bottomLeft ? 1 : 0) + (bottomRight ? 1 : 0);
	return foundCount >= 3;
}

/**
 * Extract bits from a Han Xin symbol.
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
	if (!DetectFinderPatterns(image, left, top, width, height, moduleSize))
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

	return Barcode(std::move(decRes), std::move(detRes), BarcodeFormat::HanXin);
}

} // namespace ZXing::HanXin
