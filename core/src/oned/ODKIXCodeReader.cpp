/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODKIXCodeReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "Content.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Quadrilateral.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace ZXing::OneD {

// Bar state values for 4-state postal codes
// Same encoding as Australia Post and RM4SCC
enum BarState : uint8_t {
	FULL = 0,       // F - Full height bar (ascender + tracker + descender)
	ASCENDER = 1,   // A - Top half bar (ascender + tracker)
	DESCENDER = 2,  // D - Bottom half bar (tracker + descender)
	TRACKER = 3     // T - Short middle bar only (tracker)
};

// KIX character set: 0-9 A-Z (36 characters)
static constexpr char KIX_CHARSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Royal Table: 4-state encoding for each character
// Each row represents one character with 4 bar states
// Values: 0=Full, 1=Ascender, 2=Descender, 3=Tracker
// Index 0-9 = digits '0'-'9', Index 10-35 = letters 'A'-'Z'
static constexpr uint8_t ROYAL_TABLE[36][4] = {
	{3, 3, 0, 0}, // 0
	{3, 2, 1, 0}, // 1
	{3, 2, 0, 1}, // 2
	{2, 3, 1, 0}, // 3
	{2, 3, 0, 1}, // 4
	{2, 2, 1, 1}, // 5
	{3, 1, 2, 0}, // 6
	{3, 0, 3, 0}, // 7
	{3, 0, 2, 1}, // 8
	{2, 1, 3, 0}, // 9
	{2, 1, 2, 1}, // A (10)
	{2, 0, 3, 1}, // B (11)
	{3, 1, 0, 2}, // C (12)
	{3, 0, 1, 2}, // D (13)
	{3, 0, 0, 3}, // E (14)
	{2, 1, 1, 2}, // F (15)
	{2, 1, 0, 3}, // G (16)
	{2, 0, 1, 3}, // H (17)
	{1, 3, 2, 0}, // I (18)
	{1, 2, 3, 0}, // J (19)
	{1, 2, 2, 1}, // K (20)
	{0, 3, 3, 0}, // L (21)
	{0, 3, 2, 1}, // M (22)
	{0, 2, 3, 1}, // N (23)
	{1, 3, 0, 2}, // O (24)
	{1, 2, 1, 2}, // P (25)
	{1, 2, 0, 3}, // Q (26)
	{0, 3, 1, 2}, // R (27)
	{0, 3, 0, 3}, // S (28)
	{0, 2, 1, 3}, // T (29)
	{1, 1, 2, 2}, // U (30)
	{1, 0, 3, 2}, // V (31)
	{1, 0, 2, 3}, // W (32)
	{0, 1, 3, 2}, // X (33)
	{0, 1, 2, 3}, // Y (34)
	{0, 0, 3, 3}, // Z (35)
};

// Decode 4 bar states to a character index
// Returns -1 if no match found
static int DecodeQuad(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
	for (int i = 0; i < 36; ++i) {
		if (ROYAL_TABLE[i][0] == b0 && ROYAL_TABLE[i][1] == b1 &&
		    ROYAL_TABLE[i][2] == b2 && ROYAL_TABLE[i][3] == b3) {
			return i;
		}
	}
	return -1;
}

// Structure to hold detected barcode region
struct BarcodeRegion {
	int left;
	int right;
	int top;        // Topline of barcode
	int bottom;     // Baseline of barcode
	std::vector<int> barCenters;  // X positions of each bar center
	std::vector<int> barTops;     // Top Y of each bar
	std::vector<int> barBottoms;  // Bottom Y of each bar
	float barWidth;
	float barSpacing;
	bool valid;
};

// Find the vertical extent of a bar at position x
static void FindBarExtent(const BitMatrix& image, int x, int searchTop, int searchBottom,
                          int& barTop, int& barBottom) {
	barTop = -1;
	barBottom = -1;

	// Find topmost black pixel
	for (int y = searchTop; y <= searchBottom; ++y) {
		if (image.get(x, y)) {
			barTop = y;
			break;
		}
	}

	// Find bottommost black pixel
	for (int y = searchBottom; y >= searchTop; --y) {
		if (image.get(x, y)) {
			barBottom = y;
			break;
		}
	}
}

// Detect 4-state barcode region in the image
static BarcodeRegion DetectBarcodeRegion(const BitMatrix& image, int startY) {
	BarcodeRegion region;
	region.valid = false;

	int width = image.width();
	int height = image.height();

	// Scan a horizontal band to find evenly-spaced vertical bars
	int bandHeight = std::max(3, height / 30);
	int midY = std::min(std::max(startY, bandHeight), height - bandHeight - 1);

	// Find black runs (potential bars) in the band
	std::vector<std::pair<int, int>> blackRuns;  // (start, end) x positions
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

	// KIX requires 7-24 characters, each 4 bars = 28-96 bars
	if (blackRuns.size() < 28)
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

	// Find a sequence with consistent spacing (indicating a barcode)
	size_t bestStart = 0;
	size_t bestLength = 0;
	float bestAvgSpacing = 0;

	for (size_t start = 0; start + 27 < spacings.size(); ++start) {
		// Calculate average spacing for first 28 gaps
		float avgSpacing = 0;
		for (size_t i = start; i < start + 27; ++i) {
			avgSpacing += spacings[i];
		}
		avgSpacing /= 27.0f;

		// Count how many consecutive bars have consistent spacing
		size_t count = 0;
		for (size_t i = start; i < spacings.size(); ++i) {
			float deviation = std::abs(spacings[i] - avgSpacing) / avgSpacing;
			if (deviation > 0.35f)
				break;
			count++;
		}

		if (count + 1 > bestLength && count >= 27) {
			bestStart = start;
			bestLength = count + 1;
			bestAvgSpacing = avgSpacing;
		}
	}

	// KIX: 28-96 bars (7-24 characters * 4 bars each)
	if (bestLength < 28)
		return region;

	// Validate: bar count must be multiple of 4 (each character is 4 bars)
	// and within valid range
	if (bestLength > 96) bestLength = 96;
	bestLength = (bestLength / 4) * 4;  // Round down to multiple of 4
	if (bestLength < 28)
		return region;

	// Collect the bars and find their vertical extents
	region.barCenters.clear();
	region.barTops.clear();
	region.barBottoms.clear();

	// Determine the vertical search range
	int searchTop = 0;
	int searchBottom = height - 1;

	// First pass: get rough bar positions
	for (size_t i = bestStart; i < bestStart + bestLength && i < barCenters.size(); ++i) {
		region.barCenters.push_back(barCenters[i]);
	}

	// Find vertical extent of each bar
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

// Classify bar height into one of 4 states
static BarState ClassifyBar(int barTop, int barBottom, int regionTop, int regionBottom) {
	int fullHeight = regionBottom - regionTop;
	if (fullHeight <= 0) return TRACKER;

	int barHeight = barBottom - barTop;

	// Calculate position relative to region
	float topRatio = static_cast<float>(barTop - regionTop) / fullHeight;
	float bottomRatio = static_cast<float>(regionBottom - barBottom) / fullHeight;
	float heightRatio = static_cast<float>(barHeight) / fullHeight;

	// Full bar: extends from near top to near bottom
	if (topRatio < 0.2f && bottomRatio < 0.2f && heightRatio > 0.7f) {
		return FULL;
	}

	// Ascender: starts near top, ends around middle
	if (topRatio < 0.2f && bottomRatio > 0.3f) {
		return ASCENDER;
	}

	// Descender: starts around middle, ends near bottom
	if (topRatio > 0.3f && bottomRatio < 0.2f) {
		return DESCENDER;
	}

	// Tracker: short bar in middle
	return TRACKER;
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

// Decode the bar states into content
// KIX format: just data characters, each 4 bars (no start/stop, no checksum)
static std::string DecodeBarStates(const std::vector<uint8_t>& states) {
	size_t barCount = states.size();

	// Must be multiple of 4 and between 28-96 (7-24 characters)
	if (barCount < 28 || barCount > 96 || (barCount % 4) != 0)
		return {};

	int numChars = barCount / 4;
	std::string result;
	result.reserve(numChars);

	for (int i = 0; i < numChars; ++i) {
		int barIdx = i * 4;
		int charIdx = DecodeQuad(states[barIdx], states[barIdx + 1],
		                         states[barIdx + 2], states[barIdx + 3]);
		if (charIdx < 0 || charIdx >= 36)
			return {};  // Invalid character
		result += KIX_CHARSET[charIdx];
	}

	// Validate minimum length (at least 7 characters for valid KIX)
	if (result.length() < 7)
		return {};

	return result;
}

// Try decoding in reverse direction
static std::string DecodeBarStatesReverse(const std::vector<uint8_t>& states) {
	std::vector<uint8_t> reversed(states.rbegin(), states.rend());
	return DecodeBarStates(reversed);
}

Barcode KIXCodeReader::decodeInternal(const BitMatrix& image, bool tryRotated) const {
	int height = image.height();

	// Try different vertical positions to find the barcode
	std::vector<int> scanPositions = {
		height / 2,
		height / 3,
		2 * height / 3,
		height / 4,
		3 * height / 4
	};

	for (int y : scanPositions) {
		BarcodeRegion region = DetectBarcodeRegion(image, y);
		if (!region.valid)
			continue;

		// Read bar states from the region
		auto states = ReadBarStates(region);

		// Try to decode in forward direction
		std::string content = DecodeBarStates(states);

		// If forward fails, try reverse (barcode might be upside down)
		if (content.empty()) {
			content = DecodeBarStatesReverse(states);
		}

		if (!content.empty()) {
			// Build position quadrilateral
			QuadrilateralI position;
			if (tryRotated) {
				position = QuadrilateralI{
					{region.top, image.width() - region.right},
					{region.bottom, image.width() - region.right},
					{region.bottom, image.width() - region.left},
					{region.top, image.width() - region.left}
				};
			} else {
				position = QuadrilateralI{
					{region.left, region.top},
					{region.right, region.top},
					{region.right, region.bottom},
					{region.left, region.bottom}
				};
			}

			// Symbology identifier for KIX
			// Using 'X' for postal codes, '0' for default modifier
			SymbologyIdentifier si = {'X', '0'};

			// Create Content from the decoded string
			ByteArray bytes(content);
			Content contentObj(std::move(bytes), si);

			// Create DecoderResult and DetectorResult
			DecoderResult decoderResult(std::move(contentObj));
			DetectorResult detectorResult({}, std::move(position));

			return Barcode(std::move(decoderResult), std::move(detectorResult), BarcodeFormat::KIXCode);
		}
	}

	return {};
}

Barcode KIXCodeReader::decode(const BinaryBitmap& image) const {
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

	// Try normal orientation
	Barcode result = decodeInternal(*binImg, false);
	if (result.isValid())
		return result;

	// Try rotated 90 degrees if tryRotate is enabled
	if (_opts.tryRotate()) {
		BitMatrix rotated = binImg->copy();
		rotated.rotate90();
		result = decodeInternal(rotated, true);
		if (result.isValid())
			return result;
	}

	return {};
}

Barcodes KIXCodeReader::decode(const BinaryBitmap& image, int maxSymbols) const {
	Barcodes results;
	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}
	return results;
}

} // namespace ZXing::OneD
