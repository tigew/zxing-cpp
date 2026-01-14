/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODAustraliaPostReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "GenericGF.h"
#include "ReedSolomonDecoder.h"
#include "reader/ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

namespace ZXing::OneD {

// Bar state values (encoded as 0-3)
enum BarState : uint8_t {
	TRACKER = 0,    // T - Short middle bar only
	DESCENDER = 1,  // D - Bottom half bar (baseline to center)
	ASCENDER = 2,   // A - Top half bar (center to topline)
	FULL = 3        // F - Full height bar (baseline to topline)
};

// N Table: Numeric encoding (2 bars per digit, encodes 0-9)
static constexpr uint8_t N_TABLE[10][2] = {
	{0, 0}, // 0
	{0, 1}, // 1
	{0, 2}, // 2
	{0, 3}, // 3
	{1, 0}, // 4
	{1, 1}, // 5
	{1, 2}, // 6
	{1, 3}, // 7
	{2, 0}, // 8
	{2, 1}, // 9
};

// C Table: Character encoding (3 bars per character)
// Maps 0-63 to character (space, A-Z, a-z, #, and reserved)
static constexpr uint8_t C_TABLE[64][3] = {
	{2, 2, 2}, // 0: space
	{2, 2, 0}, // 1: A
	{2, 2, 1}, // 2: B
	{2, 2, 3}, // 3: C
	{2, 0, 2}, // 4: D
	{2, 0, 0}, // 5: E
	{2, 0, 1}, // 6: F
	{2, 0, 3}, // 7: G
	{2, 1, 2}, // 8: H
	{2, 1, 0}, // 9: I
	{2, 1, 1}, // 10: J
	{2, 1, 3}, // 11: K
	{2, 3, 2}, // 12: L
	{2, 3, 0}, // 13: M
	{2, 3, 1}, // 14: N
	{2, 3, 3}, // 15: O
	{0, 2, 2}, // 16: P
	{0, 2, 0}, // 17: Q
	{0, 2, 1}, // 18: R
	{0, 2, 3}, // 19: S
	{0, 0, 2}, // 20: T
	{0, 0, 0}, // 21: U
	{0, 0, 1}, // 22: V
	{0, 0, 3}, // 23: W
	{0, 1, 2}, // 24: X
	{0, 1, 0}, // 25: Y
	{0, 1, 1}, // 26: Z
	{0, 1, 3}, // 27: a
	{0, 3, 2}, // 28: b
	{0, 3, 0}, // 29: c
	{0, 3, 1}, // 30: d
	{0, 3, 3}, // 31: e
	{1, 2, 2}, // 32: f
	{1, 2, 0}, // 33: g
	{1, 2, 1}, // 34: h
	{1, 2, 3}, // 35: i
	{1, 0, 2}, // 36: j
	{1, 0, 0}, // 37: k
	{1, 0, 1}, // 38: l
	{1, 0, 3}, // 39: m
	{1, 1, 2}, // 40: n
	{1, 1, 0}, // 41: o
	{1, 1, 1}, // 42: p
	{1, 1, 3}, // 43: q
	{1, 3, 2}, // 44: r
	{1, 3, 0}, // 45: s
	{1, 3, 1}, // 46: t
	{1, 3, 3}, // 47: u
	{3, 2, 2}, // 48: v
	{3, 2, 0}, // 49: w
	{3, 2, 1}, // 50: x
	{3, 2, 3}, // 51: y
	{3, 0, 2}, // 52: z
	{3, 0, 0}, // 53: #
	{3, 0, 1}, // 54: (reserved)
	{3, 0, 3}, // 55: (reserved)
	{3, 1, 2}, // 56: (reserved)
	{3, 1, 0}, // 57: (reserved)
	{3, 1, 1}, // 58: (reserved)
	{3, 1, 3}, // 59: (reserved)
	{3, 3, 2}, // 60: (reserved)
	{3, 3, 0}, // 61: (reserved)
	{3, 3, 1}, // 62: (reserved)
	{3, 3, 3}, // 63: (reserved)
};

// Character alphabet for C Table decoding
static constexpr char C_ALPHABET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#";

// Format Control Codes and their properties
struct FCCInfo {
	int fcc;             // Format Control Code value (e.g., 11, 45, 59, 62, 87, 92)
	int barCount;        // Total bars including start/stop
	int customerBars;    // Customer information bars (0 = none)
	bool useNTable;      // true for N encoding (numeric), false for C encoding (alphanumeric)
	const char* name;    // Human-readable name
};

static constexpr FCCInfo FCC_INFO[] = {
	{11, 37, 0, false, "Standard Customer"},
	{45, 37, 0, false, "Reply Paid"},
	{59, 52, 16, true, "Customer Barcode 2"},  // 8 numeric digits
	{62, 67, 31, false, "Customer Barcode 3"}, // ~10 alphanumeric chars
	{87, 37, 0, false, "Routing"},
	{92, 37, 0, false, "Redirection"},
};

// Find FCC info by code value
static const FCCInfo* FindFCCInfo(int fcc) {
	for (const auto& info : FCC_INFO) {
		if (info.fcc == fcc)
			return &info;
	}
	return nullptr;
}

// Decode a pair of bar states to a digit using N Table
static int DecodeNPair(uint8_t b0, uint8_t b1) {
	for (int i = 0; i < 10; ++i) {
		if (N_TABLE[i][0] == b0 && N_TABLE[i][1] == b1)
			return i;
	}
	return -1;
}

// Decode a triplet of bar states to a character index using C Table (reverse lookup)
static int DecodeCTripletIndex(uint8_t b0, uint8_t b1, uint8_t b2) {
	for (int i = 0; i < 64; ++i) {
		if (C_TABLE[i][0] == b0 && C_TABLE[i][1] == b1 && C_TABLE[i][2] == b2)
			return i;
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

	// Need at least 37 bars for shortest format
	if (blackRuns.size() < 37)
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

	for (size_t start = 0; start + 36 < spacings.size(); ++start) {
		// Calculate average spacing for first 36 gaps
		float avgSpacing = 0;
		for (size_t i = start; i < start + 36; ++i) {
			avgSpacing += spacings[i];
		}
		avgSpacing /= 36.0f;

		// Count how many consecutive bars have consistent spacing
		size_t count = 0;
		for (size_t i = start; i < spacings.size(); ++i) {
			float deviation = std::abs(spacings[i] - avgSpacing) / avgSpacing;
			if (deviation > 0.35f)
				break;
			count++;
		}

		if (count + 1 > bestLength && count >= 36) {
			bestStart = start;
			bestLength = count + 1;
			bestAvgSpacing = avgSpacing;
		}
	}

	if (bestLength < 37)
		return region;

	// Validate: check for valid bar counts (37, 52, or 67)
	if (bestLength != 37 && bestLength != 52 && bestLength != 67) {
		// Try to find exact match
		if (bestLength > 67) bestLength = 67;
		else if (bestLength > 52 && bestLength < 67) bestLength = 52;
		else if (bestLength > 37 && bestLength < 52) bestLength = 37;
	}

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
static std::string DecodeBarStates(const std::vector<uint8_t>& inputStates, const FCCInfo** outFCC) {
	*outFCC = nullptr;

	size_t barCount = inputStates.size();
	if (barCount < 37)
		return {};

	// Check start bars: must be 1,3 (Descender, Full)
	if (inputStates[0] != DESCENDER || inputStates[1] != FULL)
		return {};

	// Check stop bars: must be 1,3 (Descender, Full)
	if (inputStates[barCount - 2] != DESCENDER || inputStates[barCount - 1] != FULL)
		return {};

	// Decode FCC (Format Control Code) - 4 bars at positions 2-5
	// FCC is encoded as 2 N-table digits
	int fccDigit1 = DecodeNPair(inputStates[2], inputStates[3]);
	int fccDigit2 = DecodeNPair(inputStates[4], inputStates[5]);

	if (fccDigit1 < 0 || fccDigit2 < 0)
		return {};

	int fcc = fccDigit1 * 10 + fccDigit2;

	// Validate FCC
	const FCCInfo* fccInfo = FindFCCInfo(fcc);
	if (!fccInfo)
		return {};

	// Check bar count matches FCC
	if (static_cast<int>(barCount) != fccInfo->barCount)
		return {};

	*outFCC = fccInfo;

	// Make a local copy of states for RS correction
	std::vector<uint8_t> states = inputStates;

	// Build RS codeword for error checking
	std::vector<int> rsCodeword;
	// Convert bars to triplets (each triplet = one RS symbol)
	// Data starts after start bars (2), RS parity is last 12 bars before stop (2)
	int dataStart = 2;  // After start bars

	for (int i = dataStart; i + 2 < static_cast<int>(barCount) - 2; i += 3) {
		int value = states[i] * 16 + states[i+1] * 4 + states[i+2];
		rsCodeword.push_back(value);
	}

	if (!ReedSolomonDecode(GenericGF::MaxiCodeField64(), rsCodeword, 4))
		return {};

	// Apply corrected values back to states
	for (size_t i = 0; i < rsCodeword.size(); ++i) {
		int value = rsCodeword[i];
		int barIndex = dataStart + static_cast<int>(i) * 3;
		if (barIndex + 2 >= static_cast<int>(barCount) - 2)
			break;
		states[barIndex] = static_cast<uint8_t>((value >> 4) & 0x3);
		states[barIndex + 1] = static_cast<uint8_t>((value >> 2) & 0x3);
		states[barIndex + 2] = static_cast<uint8_t>(value & 0x3);
	}

	// Decode DPID (Delivery Point Identifier) - 8 digits at positions 6-21
	std::string dpid;
	for (int i = 0; i < 8; ++i) {
		int barIdx = 6 + i * 2;
		if (barIdx + 1 >= static_cast<int>(barCount) - 14)  // Leave room for RS and stop
			break;
		int digit = DecodeNPair(states[barIdx], states[barIdx + 1]);
		if (digit < 0)
			return {};  // Invalid DPID
		dpid += static_cast<char>('0' + digit);
	}

	if (dpid.length() != 8)
		return {};

	// Build result: [FCC][DPID][CustomerInfo]
	std::string result;
	result += std::to_string(fcc);
	result += dpid;

	// Decode customer information if present
	if (fccInfo->customerBars > 0) {
		int custStart = 22;  // After FCC (4 bars) + DPID (16 bars) + start (2)
		std::string custInfo;

		if (fccInfo->useNTable) {
			// N-table encoding: 2 bars per digit
			int numDigits = fccInfo->customerBars / 2;
			for (int i = 0; i < numDigits; ++i) {
				int barIdx = custStart + i * 2;
				if (barIdx + 1 >= static_cast<int>(barCount) - 14)
					break;
				int digit = DecodeNPair(states[barIdx], states[barIdx + 1]);
				if (digit >= 0)
					custInfo += static_cast<char>('0' + digit);
			}
		} else {
			// C-table encoding: 3 bars per character
			int numChars = fccInfo->customerBars / 3;
			for (int i = 0; i < numChars; ++i) {
				int barIdx = custStart + i * 3;
				if (barIdx + 2 >= static_cast<int>(barCount) - 14)
					break;
				int charIdx = DecodeCTripletIndex(states[barIdx], states[barIdx + 1], states[barIdx + 2]);
				if (charIdx >= 0 && charIdx < static_cast<int>(sizeof(C_ALPHABET) - 1))
					custInfo += C_ALPHABET[charIdx];
			}
		}

		result += custInfo;
	}

	return result;
}

Barcode AustraliaPostReader::decodeRow(const BitMatrix& image, int rowNumber) const {
	BarcodeRegion region = DetectBarcodeRegion(image, rowNumber);
	if (!region.valid)
		return {};

	// Read bar states from the region
	auto states = ReadBarStates(region);

	// Try to decode
	const FCCInfo* fccInfo = nullptr;
	std::string content = DecodeBarStates(states, &fccInfo);

	if (!content.empty() && fccInfo) {
		// Symbology identifier for Australia Post: X0
		SymbologyIdentifier si = {'X', '0'};

		// Use linear barcode constructor with y position, xStart, xStop
		int y = (region.top + region.bottom) / 2;
		return Barcode(content, y, region.left, region.right, BarcodeFormat::AustraliaPost, si);
	}

	return {};
}

Barcode AustraliaPostReader::decode(const BinaryBitmap& image) const {
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

	int height = binImg->height();

	// Try different vertical positions to find the barcode
	std::vector<int> scanPositions = {
		height / 2,
		height / 3,
		2 * height / 3,
		height / 4,
		3 * height / 4
	};

	for (int y : scanPositions) {
		Barcode result = decodeRow(*binImg, y);
		if (result.isValid())
			return result;
	}

	// Try rotated 90 degrees if tryRotate is enabled
	if (_opts.tryRotate()) {
		BitMatrix rotated = binImg->copy();
		rotated.rotate90();
		height = rotated.height();

		scanPositions = {
			height / 2,
			height / 3,
			2 * height / 3
		};

		for (int y : scanPositions) {
			Barcode result = decodeRow(rotated, y);
			if (result.isValid())
				return result;
		}
	}

	return {};
}

Barcodes AustraliaPostReader::decode(const BinaryBitmap& image, [[maybe_unused]] int maxSymbols) const {
	Barcodes results;
	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}
	return results;
}

} // namespace ZXing::OneD
