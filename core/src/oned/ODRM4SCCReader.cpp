/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODRM4SCCReader.h"

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
enum BarState : uint8_t {
	FULL = 0,       // F - Full height bar (ascender + tracker + descender)
	ASCENDER = 1,   // A - Top half bar (ascender + tracker)
	DESCENDER = 2,  // D - Bottom half bar (tracker + descender)
	TRACKER = 3     // T - Short middle bar only (tracker)
};

// RM4SCC character set: 0-9 A-Z (36 characters)
static constexpr char RM4SCC_CHARSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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

// Calculate RM4SCC checksum character
// Each character maps to a row (0-5) and column (0-5) in a 6x6 matrix
// Checksum = (sum of rows mod 6) * 6 + (sum of columns mod 6)
static int CalculateChecksum(const std::string& data) {
	int rowSum = 0;
	int colSum = 0;

	for (char c : data) {
		int pos = -1;
		if (c >= '0' && c <= '9') {
			pos = c - '0';
		} else if (c >= 'A' && c <= 'Z') {
			pos = c - 'A' + 10;
		}

		if (pos >= 0 && pos < 36) {
			rowSum += pos / 6;
			colSum += pos % 6;
		}
	}

	int checksumRow = rowSum % 6;
	int checksumCol = colSum % 6;
	return checksumRow * 6 + checksumCol;
}

// Validate checksum of decoded data
static bool ValidateChecksum(const std::string& dataWithChecksum) {
	if (dataWithChecksum.length() < 2)
		return false;

	std::string data = dataWithChecksum.substr(0, dataWithChecksum.length() - 1);
	char checksumChar = dataWithChecksum.back();

	int expectedIdx = CalculateChecksum(data);
	if (expectedIdx < 0 || expectedIdx >= 36)
		return false;

	return RM4SCC_CHARSET[expectedIdx] == checksumChar;
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

	// RM4SCC: Start(1) + Data(N*4) + Checksum(4) + Stop(1) = 4N + 6 bars
	// Minimum: 1 data char = 4 + 6 = 10 bars
	// Typical: 8-10 chars (postcode + DPS) = 32-40 + 6 = 38-46 bars
	if (blackRuns.size() < 10)
		return region;

	std::vector<int> barCenters;
	std::vector<int> barWidths;
	for (const auto& run : blackRuns) {
		barCenters.push_back((run.first + run.second) / 2);
		barWidths.push_back(run.second - run.first);
	}

	std::vector<int> spacings;
	for (size_t i = 1; i < barCenters.size(); ++i) {
		spacings.push_back(barCenters[i] - barCenters[i-1]);
	}

	// Find sequence with consistent spacing
	size_t bestStart = 0;
	size_t bestLength = 0;
	float bestAvgSpacing = 0;

	for (size_t start = 0; start + 9 < spacings.size(); ++start) {
		float avgSpacing = 0;
		for (size_t i = start; i < start + 9; ++i) {
			avgSpacing += spacings[i];
		}
		avgSpacing /= 9.0f;

		size_t count = 0;
		for (size_t i = start; i < spacings.size(); ++i) {
			float deviation = std::abs(spacings[i] - avgSpacing) / avgSpacing;
			if (deviation > 0.35f)
				break;
			count++;
		}

		if (count + 1 > bestLength && count >= 9) {
			bestStart = start;
			bestLength = count + 1;
			bestAvgSpacing = avgSpacing;
		}
	}

	// RM4SCC minimum: 10 bars (1 start + 4 data + 4 checksum + 1 stop)
	if (bestLength < 10)
		return region;

	// Maximum practical length
	if (bestLength > 100) bestLength = 100;

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

// Classify bar height into one of 4 states
static BarState ClassifyBar(int barTop, int barBottom, int regionTop, int regionBottom) {
	int fullHeight = regionBottom - regionTop;
	if (fullHeight <= 0) return TRACKER;

	int barHeight = barBottom - barTop;

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
// RM4SCC format: Start(Ascender) + Data(N*4 bars) + Checksum(4 bars) + Stop(Full)
static std::string DecodeBarStates(const std::vector<uint8_t>& states) {
	size_t barCount = states.size();

	// Minimum: 1 start + 4 data + 4 checksum + 1 stop = 10 bars
	// Structure: start(1) + data(N*4) + checksum(4) + stop(1) = 4N + 6
	// So (barCount - 6) must be divisible by 4
	if (barCount < 10 || ((barCount - 6) % 4) != 0)
		return {};

	// Check start bar (should be Ascender = 1)
	if (states[0] != ASCENDER)
		return {};

	// Check stop bar (should be Full = 0)
	if (states[barCount - 1] != FULL)
		return {};

	// Extract data bars (excluding start and stop)
	size_t dataBars = barCount - 2;  // Remove start and stop
	int numChars = dataBars / 4;     // Each character is 4 bars (including checksum)

	if (numChars < 2)  // At least 1 data char + 1 checksum
		return {};

	std::string result;
	result.reserve(numChars);

	for (int i = 0; i < numChars; ++i) {
		int barIdx = 1 + i * 4;  // Skip start bar
		int charIdx = DecodeQuad(states[barIdx], states[barIdx + 1],
		                         states[barIdx + 2], states[barIdx + 3]);
		if (charIdx < 0 || charIdx >= 36)
			return {};
		result += RM4SCC_CHARSET[charIdx];
	}

	// Validate checksum
	if (!ValidateChecksum(result))
		return {};

	// Return data without checksum character
	return result.substr(0, result.length() - 1);
}

// Try decoding in reverse direction
static std::string DecodeBarStatesReverse(const std::vector<uint8_t>& states) {
	// For reverse, the stop becomes start and start becomes stop
	// Also need to reverse each character's 4 bars AND the character order
	std::vector<uint8_t> reversed(states.rbegin(), states.rend());
	return DecodeBarStates(reversed);
}

Barcode RM4SCCReader::decodeInternal(const BitMatrix& image, bool tryRotated) const {
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

		// Try to decode in forward direction
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

			// Symbology identifier for RM4SCC
			SymbologyIdentifier si = {'X', '0'};

			ByteArray bytes(content);
			Content contentObj(std::move(bytes), si);

			DecoderResult decoderResult(std::move(contentObj));
			DetectorResult detectorResult({}, std::move(position));

			return Barcode(std::move(decoderResult), std::move(detectorResult), BarcodeFormat::RM4SCC);
		}
	}

	return {};
}

Barcode RM4SCCReader::decode(const BinaryBitmap& image) const {
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

Barcodes RM4SCCReader::decode(const BinaryBitmap& image, int maxSymbols) const {
	Barcodes results;
	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}
	(void)maxSymbols;  // Currently only detect one symbol
	return results;
}

} // namespace ZXing::OneD
