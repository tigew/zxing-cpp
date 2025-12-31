/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODMailmarkReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "Content.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "GenericGF.h"
#include "Quadrilateral.h"
#include "ReaderOptions.h"
#include "ReedSolomonDecoder.h"
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

// Mailmark barcode variants
enum MailmarkType {
	MAILMARK_C = 0,  // 22 chars, 66 bars, 6 RS check symbols
	MAILMARK_L = 1   // 26 chars, 78 bars, 7 RS check symbols
};

// Symbol tables for odd positions (32 values, 0-31)
static constexpr uint8_t SYMBOL_ODD[32] = {
	0x01, 0x02, 0x04, 0x07, 0x08, 0x0B, 0x0D, 0x0E,
	0x10, 0x13, 0x15, 0x16, 0x19, 0x1A, 0x1C, 0x1F,
	0x20, 0x23, 0x25, 0x26, 0x29, 0x2A, 0x2C, 0x2F,
	0x31, 0x32, 0x34, 0x37, 0x38, 0x3B, 0x3D, 0x3E
};

// Symbol tables for even positions (30 values, 0-29)
static constexpr uint8_t SYMBOL_EVEN[30] = {
	0x03, 0x05, 0x06, 0x09, 0x0A, 0x0C, 0x0F, 0x11,
	0x12, 0x14, 0x17, 0x18, 0x1B, 0x1D, 0x1E, 0x21,
	0x22, 0x24, 0x27, 0x28, 0x2B, 0x2D, 0x2E, 0x30,
	0x33, 0x35, 0x36, 0x39, 0x3A, 0x3C
};

// Extender position mapping for Barcode C (22 extenders)
// Maps from physical position to logical symbol position
static constexpr uint8_t EXTENDER_C[22] = {
	3, 5, 7, 11, 13, 14, 16, 17, 19, 0, 1, 2, 4, 6, 8, 9, 10, 12, 15, 18, 20, 21
};

// Extender position mapping for Barcode L (26 extenders)
static constexpr uint8_t EXTENDER_L[26] = {
	2, 5, 7, 8, 13, 14, 15, 16, 21, 22, 23, 0, 1, 3, 4, 6, 9, 10, 11, 12, 17, 18, 19, 20, 24, 25
};

// Character sets
static const char* SET_A = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";  // 26 letters
static const char* SET_L = "ABDEFGHJLNPQRSTUWXYZ";         // 20 letters (limited)
static const char* SET_N = "0123456789";                    // 10 digits

// Look up extender value in symbol table, return symbol index or -1
static int ExtenderToSymbol(uint8_t extender, int symbolPos) {
	if (symbolPos % 2 == 0) {
		// Even position: use SYMBOL_EVEN (30 values)
		for (int i = 0; i < 30; ++i) {
			if (SYMBOL_EVEN[i] == extender)
				return i;
		}
	} else {
		// Odd position: use SYMBOL_ODD (32 values)
		for (int i = 0; i < 32; ++i) {
			if (SYMBOL_ODD[i] == extender)
				return i;
		}
	}
	return -1;
}

// Convert 3 bar states to an extender value
// extPos: position of the extender group (affects A/D interpretation)
// bar0, bar1, bar2: the three bar states (FULL, ASCENDER, DESCENDER, TRACKER)
static uint8_t BarsToExtender(BarState bar0, BarState bar1, BarState bar2, int extPos) {
	uint8_t ext = 0;
	bool evenPos = (extPos % 2 == 0);

	// Bar 0 -> bits 5,2 of extender
	switch (bar0) {
		case FULL:
			ext |= 0x24;  // bits 5 and 2
			break;
		case ASCENDER:
			ext |= evenPos ? 0x20 : 0x04;  // bit 5 or bit 2
			break;
		case DESCENDER:
			ext |= evenPos ? 0x04 : 0x20;  // bit 2 or bit 5
			break;
		case TRACKER:
			break;  // No bits
	}

	// Bar 1 -> bits 4,1 of extender
	switch (bar1) {
		case FULL:
			ext |= 0x12;  // bits 4 and 1
			break;
		case ASCENDER:
			ext |= evenPos ? 0x10 : 0x02;  // bit 4 or bit 1
			break;
		case DESCENDER:
			ext |= evenPos ? 0x02 : 0x10;  // bit 1 or bit 4
			break;
		case TRACKER:
			break;
	}

	// Bar 2 -> bits 3,0 of extender
	switch (bar2) {
		case FULL:
			ext |= 0x09;  // bits 3 and 0
			break;
		case ASCENDER:
			ext |= evenPos ? 0x08 : 0x01;  // bit 3 or bit 0
			break;
		case DESCENDER:
			ext |= evenPos ? 0x01 : 0x08;  // bit 0 or bit 3
			break;
		case TRACKER:
			break;
	}

	return ext;
}

// CDV (Consolidated Data Value) structure for big integer operations
// Mailmark uses large integers that don't fit in 64 bits
struct CDV {
	// Store as array of limbs (base 10^9 for easy conversion)
	static constexpr int LIMB_BASE = 1000000000;
	static constexpr int MAX_LIMBS = 10;
	int limbs[MAX_LIMBS];
	int numLimbs;

	CDV() : numLimbs(0) {
		for (int i = 0; i < MAX_LIMBS; ++i)
			limbs[i] = 0;
	}

	// Initialize from symbols array (reconstruct from data)
	void fromSymbols(const std::vector<int>& symbols, int numDataSymbols) {
		// CDV = sum(symbol[i] * base^i) where base depends on position
		// Odd positions: base 32, Even positions: base 30
		// Start fresh
		numLimbs = 1;
		limbs[0] = 0;

		// Horner's method: process from most significant to least
		for (int i = 0; i < numDataSymbols; ++i) {
			int base = (i % 2 == 0) ? 30 : 32;
			multiplyAdd(base, symbols[i]);
		}
	}

	// Multiply by scalar and add value
	void multiplyAdd(int mult, int add) {
		long long carry = add;
		for (int i = 0; i < numLimbs; ++i) {
			long long val = (long long)limbs[i] * mult + carry;
			limbs[i] = val % LIMB_BASE;
			carry = val / LIMB_BASE;
		}
		while (carry > 0) {
			if (numLimbs < MAX_LIMBS) {
				limbs[numLimbs++] = carry % LIMB_BASE;
				carry /= LIMB_BASE;
			} else {
				break;
			}
		}
	}

	// Divide by scalar, return remainder
	int divideRemainder(int divisor) {
		long long rem = 0;
		for (int i = numLimbs - 1; i >= 0; --i) {
			long long val = rem * LIMB_BASE + limbs[i];
			limbs[i] = val / divisor;
			rem = val % divisor;
		}
		// Trim leading zeros
		while (numLimbs > 1 && limbs[numLimbs - 1] == 0)
			numLimbs--;
		return (int)rem;
	}

	// Check if zero
	bool isZero() const {
		return numLimbs == 1 && limbs[0] == 0;
	}
};

// Parse CDV into Mailmark fields and return formatted string
static std::string ParseCDV(CDV& cdv, MailmarkType type) {
	// Extract fields in reverse order (least significant first)
	// Field sizes depend on barcode type

	// Version ID (0-3, stored as 1-4 in display)
	int versionId = cdv.divideRemainder(4);
	if (versionId < 0 || versionId > 3)
		return {};

	// Format (0-4)
	int format = cdv.divideRemainder(5);
	if (format < 0 || format > 4)
		return {};

	// Class (0-14, maps to 0-9,A-E)
	int mailClass = cdv.divideRemainder(15);
	if (mailClass < 0 || mailClass > 14)
		return {};

	// Supply Chain ID
	int supplyChainId;
	if (type == MAILMARK_C) {
		// 2 digits (0-99)
		supplyChainId = cdv.divideRemainder(100);
	} else {
		// 6 digits (0-999999)
		supplyChainId = cdv.divideRemainder(1000000);
	}

	// Item ID (8 digits, 0-99999999)
	int itemId = cdv.divideRemainder(100000000);

	// The remaining value is the Destination Postcode encoded value
	// For now, we extract it as a numeric value
	// Full postcode decoding requires the postcode format patterns

	// Build result string
	std::string result;
	result.reserve(type == MAILMARK_C ? 22 : 26);

	// Format (single digit 0-4)
	result += ('0' + format);

	// Version ID (1-4)
	result += ('0' + versionId + 1);

	// Class (0-9 or A-E)
	if (mailClass < 10) {
		result += ('0' + mailClass);
	} else {
		result += ('A' + mailClass - 10);
	}

	// Supply Chain ID
	if (type == MAILMARK_C) {
		result += ('0' + supplyChainId / 10);
		result += ('0' + supplyChainId % 10);
	} else {
		char scid[8];
		snprintf(scid, sizeof(scid), "%06d", supplyChainId);
		result += scid;
	}

	// Item ID (8 digits)
	char itemStr[10];
	snprintf(itemStr, sizeof(itemStr), "%08d", itemId);
	result += itemStr;

	// Destination Postcode + DPS (9 chars)
	// The remaining CDV value encodes the postcode
	// We need to decode based on format
	// For simplicity, output as numeric for now
	// A full implementation would decode using postcode patterns

	// Extract postcode value from remaining CDV
	// This is a simplified version - the actual encoding is complex
	std::string postcode;
	postcode.reserve(9);

	// Decode postcode characters based on format patterns
	// Format patterns determine which character sets to use
	// Pattern: A=letters, N=digits, L=limited letters, S=space
	static const char* patterns[] = {
		"ANANLLNLS",  // Format 1
		"AANNLLNLS",  // Format 2
		"AANNNLLNL",  // Format 3
		"AANANLLNL",  // Format 4
		"ANNLLNLSS",  // Format 5
		"ANNNLLNLS",  // Format 6
		"XY11-----"   // Format 7 (international)
	};

	if (format < 7) {
		const char* pattern = patterns[format];
		// Decode each character from CDV
		for (int i = 8; i >= 0; --i) {
			char pchar = pattern[i];
			int charVal;
			switch (pchar) {
				case 'A':
					charVal = cdv.divideRemainder(26);
					postcode = std::string(1, SET_A[charVal]) + postcode;
					break;
				case 'N':
					charVal = cdv.divideRemainder(10);
					postcode = std::string(1, SET_N[charVal]) + postcode;
					break;
				case 'L':
					charVal = cdv.divideRemainder(20);
					postcode = std::string(1, SET_L[charVal]) + postcode;
					break;
				case 'S':
					postcode = " " + postcode;
					break;
				default:
					postcode = "?" + postcode;
					break;
			}
		}
	} else {
		// International format
		postcode = "XY11     ";
	}

	result += postcode;

	return result;
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

	// Mailmark: 66 bars (C) or 78 bars (L)
	if (blackRuns.size() < 66)
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

	for (size_t start = 0; start + 65 < spacings.size(); ++start) {
		float avgSpacing = 0;
		for (size_t i = start; i < start + 10; ++i) {
			avgSpacing += spacings[i];
		}
		avgSpacing /= 10.0f;

		size_t count = 0;
		for (size_t i = start; i < spacings.size(); ++i) {
			float deviation = std::abs(spacings[i] - avgSpacing) / avgSpacing;
			if (deviation > 0.35f)
				break;
			count++;
		}

		if (count + 1 > bestLength && count >= 65) {
			bestStart = start;
			bestLength = count + 1;
			bestAvgSpacing = avgSpacing;
		}
	}

	// Minimum: 66 bars for Barcode C
	if (bestLength < 66)
		return region;

	// Maximum: 78 bars for Barcode L
	if (bestLength > 78) bestLength = 78;

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
static std::vector<BarState> ReadBarStates(const BarcodeRegion& region) {
	std::vector<BarState> states;
	states.reserve(region.barCenters.size());

	for (size_t i = 0; i < region.barCenters.size(); ++i) {
		BarState state = ClassifyBar(region.barTops[i], region.barBottoms[i],
		                             region.top, region.bottom);
		states.push_back(state);
	}

	return states;
}

// Decode bar states into Mailmark content
static std::string DecodeBarStates(const std::vector<BarState>& states) {
	size_t barCount = states.size();

	// Determine barcode type
	MailmarkType type;
	int numExtenders;
	int numDataSymbols;
	int numCheckSymbols;
	const uint8_t* extenderMap;

	if (barCount == 66) {
		type = MAILMARK_C;
		numExtenders = 22;
		numDataSymbols = 16;
		numCheckSymbols = 6;
		extenderMap = EXTENDER_C;
	} else if (barCount == 78) {
		type = MAILMARK_L;
		numExtenders = 26;
		numDataSymbols = 19;
		numCheckSymbols = 7;
		extenderMap = EXTENDER_L;
	} else {
		return {};  // Invalid bar count
	}

	// Convert bars to extender values
	std::vector<uint8_t> extenders(numExtenders);
	for (int i = 0; i < numExtenders; ++i) {
		int barIdx = i * 3;
		extenders[i] = BarsToExtender(states[barIdx], states[barIdx + 1], states[barIdx + 2], i);
	}

	// Convert extenders to symbols (unshuffled order)
	std::vector<int> symbols(numExtenders);
	for (int i = 0; i < numExtenders; ++i) {
		int logicalPos = extenderMap[i];
		int symbolVal = ExtenderToSymbol(extenders[i], logicalPos);
		if (symbolVal < 0)
			return {};  // Invalid extender value
		symbols[logicalPos] = symbolVal;
	}

	// Apply Reed-Solomon error correction
	std::vector<int> message(symbols.begin(), symbols.end());
	if (!ReedSolomonDecode(GenericGF::MailmarkField64(), message, numCheckSymbols)) {
		return {};  // RS decode failed
	}

	// Extract data symbols (first numDataSymbols)
	std::vector<int> dataSymbols(message.begin(), message.begin() + numDataSymbols);

	// Convert symbols to CDV
	CDV cdv;
	cdv.fromSymbols(dataSymbols, numDataSymbols);

	// Parse CDV into formatted string
	return ParseCDV(cdv, type);
}

// Try decoding in reverse direction
static std::string DecodeBarStatesReverse(const std::vector<BarState>& states) {
	std::vector<BarState> reversed(states.rbegin(), states.rend());
	return DecodeBarStates(reversed);
}

Barcode MailmarkReader::decodeInternal(const BitMatrix& image, bool tryRotated) const {
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

			// Symbology identifier for Mailmark
			SymbologyIdentifier si = {'X', '0'};

			ByteArray bytes(content);
			Content contentObj(std::move(bytes), si);

			DecoderResult decoderResult(std::move(contentObj));
			DetectorResult detectorResult({}, std::move(position));

			return Barcode(std::move(decoderResult), std::move(detectorResult), BarcodeFormat::Mailmark);
		}
	}

	return {};
}

Barcode MailmarkReader::decode(const BinaryBitmap& image) const {
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

Barcodes MailmarkReader::decode(const BinaryBitmap& image, int maxSymbols) const {
	Barcodes results;
	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}
	(void)maxSymbols;  // Currently only detect one symbol
	return results;
}

} // namespace ZXing::OneD
