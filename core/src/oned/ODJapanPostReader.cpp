/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODJapanPostReader.h"

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

// Bar state values for Japan Post 4-state barcode
// Note: Japan Post uses 1-4 numbering, different from some other 4-state codes
enum BarState : uint8_t {
	TRACKER = 1,    // Short bar (middle only)
	ASCENDER = 2,   // Top half bar
	DESCENDER = 3,  // Bottom half bar
	FULL = 4        // Full height bar
};

// Japan Post character set: "1234567890-abcdefgh"
// Indices 0-9: digits '1'-'9', '0'
// Index 10: hyphen '-'
// Indices 11-18: control characters CC1-CC8 ('a'-'h')
static constexpr char KASUT_CHARSET[] = "1234567890-abcdefgh";

// Japan Post encoding table: each character maps to 3 bar values (1-4)
// Characters: 1,2,3,4,5,6,7,8,9,0,-,CC1,CC2,CC3,CC4,CC5,CC6,CC7,CC8
static constexpr uint8_t JAPAN_TABLE[19][3] = {
	{1, 1, 4}, // '1' (index 0)
	{1, 3, 2}, // '2' (index 1)
	{3, 1, 2}, // '3' (index 2)
	{1, 2, 3}, // '4' (index 3)
	{1, 4, 1}, // '5' (index 4)
	{3, 2, 1}, // '6' (index 5)
	{2, 1, 3}, // '7' (index 6)
	{2, 3, 1}, // '8' (index 7)
	{4, 1, 1}, // '9' (index 8)
	{1, 4, 4}, // '0' (index 9)
	{4, 1, 4}, // '-' (index 10)
	{3, 2, 4}, // CC1 'a' (index 11) - for A-J
	{3, 4, 2}, // CC2 'b' (index 12) - for K-T
	{2, 3, 4}, // CC3 'c' (index 13) - for U-Z
	{4, 3, 2}, // CC4 'd' (index 14) - Normal priority
	{2, 4, 3}, // CC5 'e' (index 15)
	{4, 2, 3}, // CC6 'f' (index 16)
	{4, 4, 1}, // CC7 'g' (index 17) - Highest priority
	{1, 1, 1}, // CC8 'h' (index 18) - Filler character
};

// Start pattern: bars 1, 3 (Tracker, Descender)
static constexpr uint8_t START_PATTERN[2] = {TRACKER, DESCENDER};

// Stop pattern: bars 3, 1 (Descender, Tracker)
static constexpr uint8_t STOP_PATTERN[2] = {DESCENDER, TRACKER};

// Fixed barcode length
static constexpr int TOTAL_BARS = 67;
static constexpr int POSTAL_CODE_BARS = 21;  // 7 digits * 3 bars
static constexpr int ADDRESS_BARS = 39;      // 13 characters * 3 bars
static constexpr int CHECK_DIGIT_BARS = 3;

// Decode 3 bar states to a character index in KASUT_CHARSET
// Returns -1 if no match found
static int DecodeTriple(uint8_t b0, uint8_t b1, uint8_t b2) {
	for (int i = 0; i < 19; ++i) {
		if (JAPAN_TABLE[i][0] == b0 && JAPAN_TABLE[i][1] == b1 && JAPAN_TABLE[i][2] == b2) {
			return i;
		}
	}
	return -1;
}

// Convert character index to actual character
static char IndexToChar(int idx) {
	if (idx < 0 || idx >= 19)
		return '\0';
	return KASUT_CHARSET[idx];
}

// Convert digit character to its index (for letter decoding)
static int CharToDigitIndex(char c) {
	if (c >= '1' && c <= '9')
		return c - '1';
	if (c == '0')
		return 9;
	return -1;
}

// Decode a letter from control code + digit
// CC1 (a) + digit -> A-J
// CC2 (b) + digit -> K-T
// CC3 (c) + digit -> U-Z
static char DecodeLetterPair(int ccIndex, int digitIndex) {
	if (ccIndex == 11) { // CC1 for A-J
		if (digitIndex >= 0 && digitIndex <= 9)
			return 'A' + digitIndex;
	} else if (ccIndex == 12) { // CC2 for K-T
		if (digitIndex >= 0 && digitIndex <= 9)
			return 'K' + digitIndex;
	} else if (ccIndex == 13) { // CC3 for U-Z
		if (digitIndex >= 0 && digitIndex <= 5)
			return 'U' + digitIndex;
	}
	return '\0';
}

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

// Detect 4-state barcode region in the image
static BarcodeRegion DetectBarcodeRegion(const BitMatrix& image, int startY) {
	BarcodeRegion region;
	region.valid = false;

	int width = image.width();
	int height = image.height();

	int bandHeight = std::max(3, height / 30);
	int midY = std::min(std::max(startY, bandHeight), height - bandHeight - 1);

	// Find black runs (potential bars) in the band
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

	// Japan Post requires exactly 67 bars
	if (blackRuns.size() < TOTAL_BARS)
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

	// Find a sequence with consistent spacing
	size_t bestStart = 0;
	size_t bestLength = 0;
	float bestAvgSpacing = 0;

	for (size_t start = 0; start + TOTAL_BARS - 1 < spacings.size(); ++start) {
		float avgSpacing = 0;
		for (size_t i = start; i < start + TOTAL_BARS - 1; ++i) {
			avgSpacing += spacings[i];
		}
		avgSpacing /= (TOTAL_BARS - 1);

		size_t count = 0;
		for (size_t i = start; i < spacings.size(); ++i) {
			float deviation = std::abs(spacings[i] - avgSpacing) / avgSpacing;
			if (deviation > 0.35f)
				break;
			count++;
		}

		if (count + 1 > bestLength && count >= TOTAL_BARS - 1) {
			bestStart = start;
			bestLength = count + 1;
			bestAvgSpacing = avgSpacing;
		}
	}

	// Must have exactly 67 bars for Japan Post
	if (bestLength < TOTAL_BARS)
		return region;
	bestLength = TOTAL_BARS;

	// Collect the bars and find their vertical extents
	region.barCenters.clear();
	region.barTops.clear();
	region.barBottoms.clear();

	int searchTop = 0;
	int searchBottom = height - 1;

	for (size_t i = bestStart; i < bestStart + bestLength && i < barCenters.size(); ++i) {
		region.barCenters.push_back(barCenters[i]);
	}

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

// Classify bar height into one of 4 states (using Japan Post numbering 1-4)
static BarState ClassifyBar(int barTop, int barBottom, int regionTop, int regionBottom) {
	int fullHeight = regionBottom - regionTop;
	if (fullHeight <= 0) return TRACKER;

	int barHeight = barBottom - barTop;

	float topRatio = static_cast<float>(barTop - regionTop) / fullHeight;
	float bottomRatio = static_cast<float>(regionBottom - barBottom) / fullHeight;
	float heightRatio = static_cast<float>(barHeight) / fullHeight;

	// Full bar (4): extends from near top to near bottom
	if (topRatio < 0.2f && bottomRatio < 0.2f && heightRatio > 0.7f) {
		return FULL;
	}

	// Ascender (2): starts near top, ends around middle
	if (topRatio < 0.2f && bottomRatio > 0.3f) {
		return ASCENDER;
	}

	// Descender (3): starts around middle, ends near bottom
	if (topRatio > 0.3f && bottomRatio < 0.2f) {
		return DESCENDER;
	}

	// Tracker (1): short bar in middle
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

// Calculate check digit (modulo 19)
static int CalculateCheckSum(const std::vector<int>& charIndices) {
	int sum = 0;
	for (int idx : charIndices) {
		sum += idx;
	}
	int check = 19 - (sum % 19);
	if (check == 19)
		check = 0;
	return check;
}

// Decode the bar states into content
static std::string DecodeBarStates(const std::vector<uint8_t>& states) {
	if (states.size() != TOTAL_BARS)
		return {};

	// Check start pattern (bars 1, 3)
	if (states[0] != START_PATTERN[0] || states[1] != START_PATTERN[1])
		return {};

	// Check stop pattern (bars 3, 1)
	if (states[TOTAL_BARS - 2] != STOP_PATTERN[0] || states[TOTAL_BARS - 1] != STOP_PATTERN[1])
		return {};

	// Decode all characters (postal code + address + check digit)
	// Total: 21 + 39 + 3 = 63 bars for data (excluding start/stop)
	std::vector<int> charIndices;
	std::string result;

	int dataStart = 2;  // After start pattern
	int dataEnd = TOTAL_BARS - 2 - CHECK_DIGIT_BARS;  // Before check digit and stop

	// First pass: decode raw character indices
	for (int i = dataStart; i < dataEnd; i += 3) {
		if (i + 2 >= TOTAL_BARS - 2)
			break;
		int idx = DecodeTriple(states[i], states[i + 1], states[i + 2]);
		if (idx < 0)
			return {};  // Invalid character
		charIndices.push_back(idx);
	}

	// Decode check digit
	int checkStart = TOTAL_BARS - 2 - CHECK_DIGIT_BARS;
	int checkIdx = DecodeTriple(states[checkStart], states[checkStart + 1], states[checkStart + 2]);
	if (checkIdx < 0)
		return {};

	// Verify checksum
	int expectedCheck = CalculateCheckSum(charIndices);
	if (checkIdx != expectedCheck)
		return {};  // Checksum mismatch

	// Second pass: convert character indices to actual characters
	// Handle control codes for letters
	size_t pos = 0;
	while (pos < charIndices.size()) {
		int idx = charIndices[pos];
		char c = IndexToChar(idx);

		if (c >= '0' && c <= '9') {
			// Digit
			result += c;
			pos++;
		} else if (c == '-') {
			// Hyphen
			result += c;
			pos++;
		} else if (c >= 'a' && c <= 'c') {
			// Control code for letter - need next character
			if (pos + 1 >= charIndices.size())
				break;
			int nextIdx = charIndices[pos + 1];
			char nextC = IndexToChar(nextIdx);
			int digitIdx = CharToDigitIndex(nextC);
			if (digitIdx >= 0) {
				char letter = DecodeLetterPair(idx, digitIdx);
				if (letter != '\0') {
					result += letter;
				}
			}
			pos += 2;
		} else if (c == 'h') {
			// Filler character (CC8) - skip
			pos++;
		} else {
			// Other control codes (d-g) - skip for now
			pos++;
		}
	}

	// Japan Post barcode should have at least a 7-digit postal code
	if (result.length() < 7)
		return {};

	return result;
}

// Try decoding in reverse direction
static std::string DecodeBarStatesReverse(const std::vector<uint8_t>& states) {
	std::vector<uint8_t> reversed(states.rbegin(), states.rend());
	return DecodeBarStates(reversed);
}

Barcode JapanPostReader::decodeInternal(const BitMatrix& image, bool tryRotated) const {
	int height = image.height();

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

		auto states = ReadBarStates(region);

		// Try forward direction
		std::string content = DecodeBarStates(states);

		// If forward fails, try reverse
		if (content.empty()) {
			content = DecodeBarStatesReverse(states);
		}

		if (!content.empty()) {
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

			// Symbology identifier for Japan Post
			SymbologyIdentifier si = {'X', '0'};

			ByteArray bytes(content);
			Content contentObj(std::move(bytes), si);

			DecoderResult decoderResult(std::move(contentObj));
			DetectorResult detectorResult({}, std::move(position));

			return Barcode(std::move(decoderResult), std::move(detectorResult), BarcodeFormat::JapanPost);
		}
	}

	return {};
}

Barcode JapanPostReader::decode(const BinaryBitmap& image) const {
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

	Barcode result = decodeInternal(*binImg, false);
	if (result.isValid())
		return result;

	if (_opts.tryRotate()) {
		BitMatrix rotated = binImg->copy();
		rotated.rotate90();
		result = decodeInternal(rotated, true);
		if (result.isValid())
			return result;
	}

	return {};
}

Barcodes JapanPostReader::decode(const BinaryBitmap& image, [[maybe_unused]] int maxSymbols) const {
	Barcodes results;
	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}
	return results;
}

} // namespace ZXing::OneD
