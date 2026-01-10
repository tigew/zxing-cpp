/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODPharmacodeTwoTrackReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ZXing::OneD {

// Bar state values for 3-state Pharmacode Two-Track
enum BarState : uint8_t {
	FULL = 0,       // Full height bar (extends above and below center)
	ASCENDER = 1,   // Upper half bar only
	DESCENDER = 2,  // Lower half bar only
	INVALID = 3     // Invalid bar state
};

// Pharmacode Two-Track constants
static constexpr int MIN_BARS = 2;
static constexpr int MAX_BARS = 16;
static constexpr int64_t MIN_VALUE = 4;
static constexpr int64_t MAX_VALUE = 64570080;

// Structure to hold detected barcode region
struct BarcodeRegion {
	int left;
	int right;
	int top;
	int bottom;
	std::vector<int> barCenters;
	std::vector<int> barTops;
	std::vector<int> barBottoms;
	float barWidth;
	float barSpacing;
	bool valid;
};

// Find the vertical extent of a bar at position x
static void FindBarExtent(const BitMatrix& image, int x, int searchTop, int searchBottom,
                          int& barTop, int& barBottom) {
	barTop = -1;
	barBottom = -1;

	for (int y = searchTop; y <= searchBottom; ++y) {
		if (image.get(x, y)) {
			barTop = y;
			break;
		}
	}

	for (int y = searchBottom; y >= searchTop; --y) {
		if (image.get(x, y)) {
			barBottom = y;
			break;
		}
	}
}

// Detect Pharmacode Two-Track barcode region in the image
static BarcodeRegion DetectBarcodeRegion(const BitMatrix& image, int startY) {
	BarcodeRegion region;
	region.valid = false;

	int width = image.width();
	int height = image.height();

	// Search band around the start Y position
	int bandHeight = std::max(3, height / 30);
	int midY = std::min(std::max(startY, bandHeight), height - bandHeight - 1);

	// Find black runs (bars) in the middle band
	std::vector<std::pair<int, int>> blackRuns;
	bool inBlack = false;
	int runStart = 0;

	for (int x = 0; x < width; ++x) {
		bool hasBlack = false;
		for (int dy = -bandHeight/2; dy <= bandHeight/2; ++dy) {
			int y = midY + dy;
			if (y >= 0 && y < height && image.get(x, y)) {
				hasBlack = true;
				break;
			}
		}

		if (hasBlack && !inBlack) {
			runStart = x;
			inBlack = true;
		} else if (!hasBlack && inBlack) {
			blackRuns.push_back({runStart, x});
			inBlack = false;
		}
	}
	if (inBlack) {
		blackRuns.push_back({runStart, width});
	}

	// Pharmacode Two-Track: 2-16 bars
	if (blackRuns.size() < MIN_BARS)
		return region;

	// Calculate bar centers and widths
	std::vector<int> barCenters;
	std::vector<int> barWidths;
	for (const auto& run : blackRuns) {
		barCenters.push_back((run.first + run.second) / 2);
		barWidths.push_back(run.second - run.first);
	}

	// Calculate spacings between bars
	std::vector<int> spacings;
	for (size_t i = 1; i < barCenters.size(); ++i) {
		spacings.push_back(barCenters[i] - barCenters[i-1]);
	}

	// Find sequence with consistent spacing
	size_t bestStart = 0;
	size_t bestLength = 0;
	float bestAvgSpacing = 0;

	for (size_t start = 0; start + 1 < spacings.size(); ++start) {
		float avgSpacing = 0;
		size_t sampleCount = std::min(static_cast<size_t>(5), spacings.size() - start);
		for (size_t i = start; i < start + sampleCount; ++i) {
			avgSpacing += spacings[i];
		}
		avgSpacing /= sampleCount;

		size_t count = 0;
		for (size_t i = start; i < spacings.size(); ++i) {
			float deviation = std::abs(spacings[i] - avgSpacing) / avgSpacing;
			if (deviation > 0.4f)
				break;
			count++;
		}

		if (count + 1 > bestLength && count >= 1) {
			bestStart = start;
			bestLength = count + 1;
			bestAvgSpacing = avgSpacing;
		}
	}

	// Need at least 2 bars
	if (bestLength < MIN_BARS)
		return region;

	// Cap at maximum bars
	if (bestLength > MAX_BARS)
		bestLength = MAX_BARS;

	region.barCenters.clear();
	region.barTops.clear();
	region.barBottoms.clear();

	int searchTop = 0;
	int searchBottom = height - 1;

	for (size_t i = bestStart; i < bestStart + bestLength && i < barCenters.size(); ++i) {
		region.barCenters.push_back(barCenters[i]);
	}

	// Find vertical extents for each bar
	int minTop = height, maxBottom = 0;
	for (int x : region.barCenters) {
		int barTop, barBottom;
		FindBarExtent(image, x, searchTop, searchBottom, barTop, barBottom);
		if (barTop >= 0 && barBottom >= 0) {
			region.barTops.push_back(barTop);
			region.barBottoms.push_back(barBottom);
			minTop = std::min(minTop, barTop);
			maxBottom = std::max(maxBottom, barBottom);
		} else {
			region.barTops.push_back(midY - 10);
			region.barBottoms.push_back(midY + 10);
		}
	}

	region.left = region.barCenters.front() - 5;
	region.right = region.barCenters.back() + 5;
	region.top = minTop;
	region.bottom = maxBottom;
	region.barSpacing = bestAvgSpacing;
	region.barWidth = 0;
	for (size_t i = bestStart; i < bestStart + bestLength && i < barWidths.size(); ++i) {
		region.barWidth += barWidths[i];
	}
	region.barWidth /= bestLength;
	region.valid = true;

	return region;
}

// Classify bar height into one of 3 states for Pharmacode Two-Track
static BarState ClassifyBar(int barTop, int barBottom, int regionTop, int regionBottom) {
	int fullHeight = regionBottom - regionTop;
	if (fullHeight <= 0) return INVALID;

	int barHeight = barBottom - barTop;
	int midPoint = (regionTop + regionBottom) / 2;

	float topRatio = static_cast<float>(barTop - regionTop) / fullHeight;
	float bottomRatio = static_cast<float>(regionBottom - barBottom) / fullHeight;
	float heightRatio = static_cast<float>(barHeight) / fullHeight;

	// Full bar: extends from near top to near bottom (>70% height)
	if (topRatio < 0.25f && bottomRatio < 0.25f && heightRatio > 0.6f) {
		return FULL;
	}

	// Ascender (upper half): starts near top, ends around middle
	if (topRatio < 0.25f && barBottom <= midPoint + fullHeight * 0.15f) {
		return ASCENDER;
	}

	// Descender (lower half): starts around middle, ends near bottom
	if (barTop >= midPoint - fullHeight * 0.15f && bottomRatio < 0.25f) {
		return DESCENDER;
	}

	// Fallback based on center position
	int barCenter = (barTop + barBottom) / 2;
	if (barCenter < midPoint) {
		return ASCENDER;
	} else {
		return DESCENDER;
	}
}

// Read bar states from a detected region
static std::vector<uint8_t> ReadBarStates(const BarcodeRegion& region) {
	std::vector<uint8_t> states;
	states.reserve(region.barCenters.size());

	for (size_t i = 0; i < region.barCenters.size(); ++i) {
		BarState state = ClassifyBar(region.barTops[i], region.barBottoms[i],
		                             region.top, region.bottom);
		states.push_back(static_cast<uint8_t>(state));
	}

	return states;
}

// Decode bar states to numeric value using bijective base-3
// Reading from right to left:
// - Full bar: digit 1
// - Descender: digit 2
// - Ascender: digit 3
// Value = sum of (digit × 3^position)
static int64_t DecodeBarStates(const std::vector<uint8_t>& states) {
	if (states.size() < MIN_BARS || states.size() > MAX_BARS)
		return -1;

	int64_t value = 0;
	int64_t power = 1; // 3^0

	// Read from right to left
	for (int i = static_cast<int>(states.size()) - 1; i >= 0; --i) {
		int digit = 0;
		switch (states[i]) {
			case FULL:
				digit = 1;
				break;
			case DESCENDER:
				digit = 2;
				break;
			case ASCENDER:
				digit = 3;
				break;
			default:
				return -1; // Invalid bar state
		}

		value += digit * power;
		power *= 3;
	}

	return value;
}

Barcode PharmacodeTwoTrackReader::decodeInternal(const BitMatrix& image, bool tryRotated) const
{
	int height = image.height();
	int width = image.width();

	// Search multiple horizontal bands
	std::vector<int> searchRows;
	searchRows.push_back(height / 2);
	searchRows.push_back(height / 3);
	searchRows.push_back(2 * height / 3);
	searchRows.push_back(height / 4);
	searchRows.push_back(3 * height / 4);

	for (int startY : searchRows) {
		BarcodeRegion region = DetectBarcodeRegion(image, startY);
		if (!region.valid)
			continue;

		std::vector<uint8_t> states = ReadBarStates(region);
		if (states.size() < MIN_BARS)
			continue;

		// Decode value
		int64_t value = DecodeBarStates(states);
		if (value < MIN_VALUE || value > MAX_VALUE)
			continue;

		// Success
		std::string txt = std::to_string(value);

		// Build position info
		int xStart = region.left;
		int xStop = region.right;

		// Pharmacode Two-Track uses 'L' for Laetus symbology identifier
		SymbologyIdentifier symbologyIdentifier = {'L', '1'};

		return {std::move(txt), startY, xStart, xStop, BarcodeFormat::PharmacodeTwoTrack, symbologyIdentifier};
	}

	return {};
}

Barcode PharmacodeTwoTrackReader::decode(const BinaryBitmap& image) const
{
	auto results = decode(image, 1);
	return results.empty() ? Barcode() : std::move(results.front());
}

Barcodes PharmacodeTwoTrackReader::decode(const BinaryBitmap& image, int maxSymbols) const
{
	Barcodes results;

	auto binImg = image.getBitMatrix();
	if (!binImg)
		return results;

	// Try normal orientation
	auto barcode = decodeInternal(*binImg, false);
	if (barcode.isValid()) {
		results.push_back(std::move(barcode));
		if (maxSymbols == 1)
			return results;
	}

	// Try rotated if enabled
	if (_opts.tryRotate()) {
		BitMatrix rotated = binImg->copy();
		rotated.rotate90();
		barcode = decodeInternal(rotated, true);
		if (barcode.isValid()) {
			results.push_back(std::move(barcode));
		}
	}

	return results;
}

} // namespace ZXing::OneD
