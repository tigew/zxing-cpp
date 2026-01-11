/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2026 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "MCDetector.h"

#include "BitMatrix.h"
#include "ConcentricFinder.h"
#include "DetectorResult.h"
#include "GridSampler.h"
#include "MCBitMatrixParser.h"
#include "Pattern.h"
#include "Quadrilateral.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace ZXing::MaxiCode {

/**
 * MaxiCode has a distinctive bullseye finder pattern consisting of 3 concentric circles.
 * The pattern from center outward is approximately: black center, white ring, black ring, white ring
 * In terms of module widths, the pattern is roughly 3:3:3:3 (4 transitions, similar to Aztec's pattern
 * but circular rather than square).
 *
 * For detection purposes, we look for this concentric circular pattern.
 * The bullseye occupies approximately the center 10-12 modules of the 30x33 grid.
 */

static constexpr int MATRIX_WIDTH = BitMatrixParser::MATRIX_WIDTH;
static constexpr int MATRIX_HEIGHT = BitMatrixParser::MATRIX_HEIGHT;

/**
 * Detects MaxiCode in pure images (no rotation/skew)
 */
static ZXing::DetectorResult ExtractPureBits(const BitMatrix& image)
{
	int left, top, width, height;
	if (!image.findBoundingBox(left, top, width, height, MATRIX_WIDTH))
		return {};

	// Now just read off the bits with hexagonal offset
	BitMatrix bits(MATRIX_WIDTH, MATRIX_HEIGHT);
	for (int y = 0; y < MATRIX_HEIGHT; y++) {
		int iy = top + (y * height + height / 2) / MATRIX_HEIGHT;
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			// Hexagonal grid: odd rows are offset by half a module width
			int ix = left + (x * width + width / 2 + (y & 0x01) * width / 2) / MATRIX_WIDTH;
			if (image.get(ix, iy)) {
				bits.set(x, y);
			}
		}
	}

	return {std::move(bits), Rectangle<PointI>(left, top, width, height)};
}

/**
 * Find the MaxiCode bullseye finder pattern in a pure image
 */
static std::optional<ConcentricPattern> FindPureFinderPattern(const BitMatrix& image)
{
	int left, top, width, height;
	// MaxiCode is 30x33 modules, look for a bounding box of at least that size
	if (!image.findBoundingBox(left, top, width, height, MATRIX_WIDTH))
		return {};

	// Center should be approximately in the middle of the bounding box
	PointF center(left + width / 2.0f, top + height / 2.0f);

	// MaxiCode bullseye has roughly 4 rings (black, white, black, white from center)
	// The pattern is approximately 1:1:1:1 (though circular, not square)
	// We use a 5-element pattern to capture: white border, black ring, white ring, black ring, black center
	constexpr auto PATTERN = FixedPattern<5, 7>{1, 1, 1, 1, 1};

	int range = std::max(width, height);
	if (auto pattern = LocateConcentricPattern(image, PATTERN, center, range))
		return pattern;

	return {};
}

/**
 * Find MaxiCode finder patterns in a general image (with rotation/skew)
 */
static std::vector<ConcentricPattern> FindFinderPatterns(const BitMatrix& image, bool tryHarder)
{
	std::vector<ConcentricPattern> res;

	// MaxiCode bullseye pattern: approximately 5 alternating rings
	// Pattern represents: outer white, black, white, black, center black
	constexpr auto PATTERN = FixedPattern<5, 7>{1, 1, 1, 1, 1};

	int skip = tryHarder ? 1 : std::clamp(image.height() / 100, 2, 4);
	int margin = skip;

	// Scan horizontally looking for the bullseye pattern
	for (int y = margin; y < image.height() - margin; y += skip) {
		auto row = image.row(y);
		PatternRow patterns;
		GetPatternRow(row, patterns);

		PatternView view(patterns);
		// Look for sequences that might be the bullseye
		for (size_t i = 2; i < patterns.size() - 2; ++i) {
			// Check if this could be the center of a concentric pattern
			PatternView window = view.subView(i - 2, 5);

			// Rough check: the pattern should be relatively uniform (circular)
			if (!IsPattern<>(window, PATTERN))
				continue;

			// Calculate approximate center position
			PointF center(window.pixelsInFront() + Reduce(window) / 2.0f, y + 0.5f);
			int range = Reduce(window) * 2;

			// Verify this is actually a concentric pattern by checking vertically and diagonally
			if (auto pattern = LocateConcentricPattern(image, PATTERN, center, range)) {
				// Check if we already found this pattern (deduplication)
				bool duplicate = false;
				for (const auto& existing : res) {
					if (distance(*pattern, existing) < pattern->size / 2) {
						duplicate = true;
						break;
					}
				}
				if (!duplicate)
					res.push_back(*pattern);
			}
		}
	}

	return res;
}

/**
 * Sample the grid from a detected finder pattern
 */
static ZXing::DetectorResult SampleGrid(const BitMatrix& image, const ConcentricPattern& center)
{
	// MaxiCode has a fixed size: 30x33 modules
	// The bullseye is approximately 10-12 modules in diameter
	// We need to estimate the module size and find the corners

	float moduleSize = center.size / 10.0f; // Bullseye is roughly 10 modules across

	// Calculate the approximate dimensions in pixels
	float halfWidth = (MATRIX_WIDTH / 2.0f) * moduleSize;
	float halfHeight = (MATRIX_HEIGHT / 2.0f) * moduleSize;

	// Define the four corners of the MaxiCode symbol
	// MaxiCode is slightly wider than it is tall (30x33 modules)
	QuadrilateralF quad = {
		PointF(center.x - halfWidth, center.y - halfHeight),  // top-left
		PointF(center.x + halfWidth, center.y - halfHeight),  // top-right
		PointF(center.x + halfWidth, center.y + halfHeight),  // bottom-right
		PointF(center.x - halfWidth, center.y + halfHeight)   // bottom-left
	};

	// Sample the grid accounting for hexagonal layout
	BitMatrix bits(MATRIX_WIDTH, MATRIX_HEIGHT);

	for (int y = 0; y < MATRIX_HEIGHT; y++) {
		for (int x = 0; x < MATRIX_WIDTH; x++) {
			// Calculate position in module coordinates (0 to MATRIX_WIDTH/HEIGHT)
			// Odd rows are offset by 0.5 modules to the right (hexagonal grid)
			float mx = x + (y & 0x01) * 0.5f;
			float my = y;

			// Map module coordinates to pixel coordinates using bilinear interpolation
			// mx ranges from 0 to MATRIX_WIDTH, my ranges from 0 to MATRIX_HEIGHT
			float px = quad[0].x + (quad[1].x - quad[0].x) * mx / MATRIX_WIDTH +
			           (quad[3].x - quad[0].x) * my / MATRIX_HEIGHT;
			float py = quad[0].y + (quad[1].y - quad[0].y) * mx / MATRIX_WIDTH +
			           (quad[3].y - quad[0].y) * my / MATRIX_HEIGHT;

			// Sample the image at this pixel position
			if (image.get(PointI(px, py))) {
				bits.set(x, y);
			}
		}
	}

	// Convert QuadrilateralF to QuadrilateralI for DetectorResult
	QuadrilateralI quadInt = {
		PointI(quad[0]),
		PointI(quad[1]),
		PointI(quad[2]),
		PointI(quad[3])
	};

	return {std::move(bits), std::move(quadInt)};
}

ZXing::DetectorResult Detect(const BitMatrix& image, bool isPure, bool tryHarder)
{
	// For pure images, use the fast extraction method
	if (isPure)
		return ExtractPureBits(image);

	// Try to find the finder pattern
	auto pattern = FindPureFinderPattern(image);

	// If pure detection failed, try the more robust pattern search
	if (!pattern) {
		auto patterns = FindFinderPatterns(image, tryHarder);
		if (patterns.empty())
			return {};

		// Use the first (or best) pattern found
		// TODO: Could rank patterns by quality/size if multiple found
		pattern = patterns[0];
	}

	// Sample the grid from the detected pattern
	return SampleGrid(image, *pattern);
}

} // namespace ZXing::MaxiCode
