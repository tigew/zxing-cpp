/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODCode16KReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "ODCode128Patterns.h"
#include "Pattern.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace ZXing::OneD {

// Code 128 constants (same as used in Code 16K)
static const int CODE_CODE_C = 99;
static const int CODE_CODE_B = 100;
static const int CODE_CODE_A = 101;
static const int CODE_FNC_1 = 102;
static const int CODE_FNC_2 = 97;
static const int CODE_FNC_3 = 96;
static const int CODE_FNC_4_A = 101;
static const int CODE_FNC_4_B = 100;
static const int CODE_SHIFT = 98;

// Code 16K specific constants
static const int MIN_ROWS = 2;
static const int MAX_ROWS = 16;
static const int CODEWORDS_PER_ROW = 5;

// Code 16K Start/Stop patterns (4 elements each, representing bar widths)
// These are different from Code 128 and indexed by row position
static const std::array<std::array<int, 4>, 8> C16K_START_STOP = {{
	{3, 2, 1, 1}, // 0
	{2, 2, 2, 1}, // 1
	{2, 1, 2, 2}, // 2
	{1, 4, 1, 1}, // 3
	{1, 1, 3, 2}, // 4
	{1, 2, 3, 1}, // 5
	{1, 1, 1, 4}, // 6
	{3, 1, 1, 2}, // 7
}};

// Start pattern index for each row (0-15)
static const std::array<int, 16> C16K_START_VALUES = {
	0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7
};

// Stop pattern index for each row (0-15)
static const std::array<int, 16> C16K_STOP_VALUES = {
	0, 1, 2, 3, 4, 5, 6, 7, 4, 5, 6, 7, 0, 1, 2, 3
};

static const float MAX_AVG_VARIANCE = 0.25f;

// Decode a Code 128 pattern (6 elements) to a codeword value
static int DecodeCodeword(const std::array<int, 6>& counters)
{
	float bestVariance = MAX_AVG_VARIANCE;
	int bestMatch = -1;

	for (size_t i = 0; i < Code128::CODE_PATTERNS.size() - 1; ++i) { // Exclude stop pattern
		const auto& pattern = Code128::CODE_PATTERNS[i];
		float variance = 0;
		int total = 0;
		int patternTotal = 0;

		for (int j = 0; j < 6; ++j) {
			total += counters[j];
			patternTotal += pattern[j];
		}

		if (total == 0 || patternTotal == 0)
			continue;

		float unitSize = static_cast<float>(total) / patternTotal;

		for (int j = 0; j < 6; ++j) {
			float expected = pattern[j] * unitSize;
			float diff = std::abs(counters[j] - expected);
			variance += diff / expected;
		}
		variance /= 6;

		if (variance < bestVariance) {
			bestVariance = variance;
			bestMatch = static_cast<int>(i);
		}
	}

	return bestMatch;
}

// Decode a start/stop pattern (4 elements)
static int DecodeStartStop(const std::array<int, 4>& counters)
{
	float bestVariance = MAX_AVG_VARIANCE;
	int bestMatch = -1;

	for (size_t i = 0; i < C16K_START_STOP.size(); ++i) {
		const auto& pattern = C16K_START_STOP[i];
		float variance = 0;
		int total = 0;
		int patternTotal = 0;

		for (int j = 0; j < 4; ++j) {
			total += counters[j];
			patternTotal += pattern[j];
		}

		if (total == 0 || patternTotal == 0)
			continue;

		float unitSize = static_cast<float>(total) / patternTotal;

		for (int j = 0; j < 4; ++j) {
			float expected = pattern[j] * unitSize;
			float diff = std::abs(counters[j] - expected);
			variance += diff / expected;
		}
		variance /= 4;

		if (variance < bestVariance) {
			bestVariance = variance;
			bestMatch = static_cast<int>(i);
		}
	}

	return bestMatch;
}

// Structure to hold a decoded row
struct Code16KRow {
	int rowIndex = -1;  // Detected row position (0-15)
	std::vector<int> codewords;  // 5 codewords per row
	int startPattern = -1;
	int stopPattern = -1;
	bool isValid = false;
};

// Decode a single row from pattern data
static Code16KRow DecodeRow(const PatternRow& bars)
{
	Code16KRow result;

	// Minimum elements: start(4) + separator(1) + 5*codewords(30) + stop(4) = 39
	if (bars.size() < 39)
		return result;

	size_t pos = 0;

	// Skip initial quiet zone (white space)
	while (pos < bars.size() && bars[pos] == 0)
		pos++;

	if (pos >= bars.size())
		return result;

	// Read start pattern (4 elements)
	std::array<int, 4> startCounters = {};
	for (int i = 0; i < 4 && pos + i < bars.size(); ++i)
		startCounters[i] = bars[pos + i];

	result.startPattern = DecodeStartStop(startCounters);
	if (result.startPattern < 0)
		return result;

	pos += 4;

	// Skip separator bar (1 element)
	if (pos >= bars.size())
		return result;
	pos += 1;

	// Read 5 codewords (6 elements each)
	for (int cw = 0; cw < CODEWORDS_PER_ROW; ++cw) {
		if (pos + 6 > bars.size())
			return result;

		std::array<int, 6> counters = {};
		for (int i = 0; i < 6; ++i)
			counters[i] = bars[pos + i];

		int code = DecodeCodeword(counters);
		if (code < 0)
			return result;

		result.codewords.push_back(code);
		pos += 6;
	}

	// Read stop pattern (4 elements)
	if (pos + 4 > bars.size())
		return result;

	std::array<int, 4> stopCounters = {};
	for (int i = 0; i < 4 && pos + i < bars.size(); ++i)
		stopCounters[i] = bars[pos + i];

	result.stopPattern = DecodeStartStop(stopCounters);
	if (result.stopPattern < 0)
		return result;

	// Determine row index from start/stop patterns
	for (int row = 0; row < MAX_ROWS; ++row) {
		if (C16K_START_VALUES[row] == result.startPattern &&
			C16K_STOP_VALUES[row] == result.stopPattern) {
			result.rowIndex = row;
			break;
		}
	}

	if (result.rowIndex >= 0)
		result.isValid = true;

	return result;
}

// Get pattern row from image
static PatternRow GetPatternRow(const BitMatrix& image, int y)
{
	PatternRow result;
	int width = image.width();

	if (y < 0 || y >= image.height())
		return result;

	result.reserve(width / 2);

	bool lastBit = image.get(0, y);
	int count = 1;

	for (int x = 1; x < width; ++x) {
		bool bit = image.get(x, y);
		if (bit == lastBit) {
			count++;
		} else {
			result.push_back(count);
			count = 1;
			lastBit = bit;
		}
	}
	result.push_back(count);

	return result;
}

// Decode character from codeword based on current code set
static void DecodeCharacter(int code, int& codeSet, std::string& result, bool& fnc4Active, bool& shift)
{
	bool wasShift = shift;
	shift = false;

	int effectiveCodeSet = codeSet;
	if (wasShift) {
		effectiveCodeSet = (codeSet == CODE_CODE_A) ? CODE_CODE_B : CODE_CODE_A;
	}

	if (effectiveCodeSet == CODE_CODE_C) {
		if (code < 100) {
			// Two-digit numeric
			result += static_cast<char>('0' + (code / 10));
			result += static_cast<char>('0' + (code % 10));
		} else if (code == CODE_CODE_A) {
			codeSet = CODE_CODE_A;
		} else if (code == CODE_CODE_B) {
			codeSet = CODE_CODE_B;
		}
	} else if (effectiveCodeSet == CODE_CODE_A) {
		if (code < 64) {
			char c = static_cast<char>(code + 32);
			if (fnc4Active) c = static_cast<char>(c + 128);
			result += c;
		} else if (code < 96) {
			char c = static_cast<char>(code - 64);
			if (fnc4Active) c = static_cast<char>(c + 128);
			result += c;
		} else if (code == CODE_SHIFT) {
			shift = true;
		} else if (code == CODE_CODE_B) {
			codeSet = CODE_CODE_B;
		} else if (code == CODE_CODE_C) {
			codeSet = CODE_CODE_C;
		} else if (code == CODE_FNC_4_A) {
			fnc4Active = !fnc4Active;
		}
	} else { // CODE_CODE_B
		if (code < 96) {
			char c = static_cast<char>(code + 32);
			if (fnc4Active) c = static_cast<char>(c + 128);
			result += c;
		} else if (code == CODE_SHIFT) {
			shift = true;
		} else if (code == CODE_CODE_A) {
			codeSet = CODE_CODE_A;
		} else if (code == CODE_CODE_C) {
			codeSet = CODE_CODE_C;
		} else if (code == CODE_FNC_4_B) {
			fnc4Active = !fnc4Active;
		}
	}
}

// Validate modulo-107 check digits
static bool ValidateCheckDigits(const std::vector<int>& allCodewords)
{
	if (allCodewords.size() < 3)
		return false;

	// Calculate first check digit
	int firstSum = 0;
	for (size_t i = 0; i < allCodewords.size() - 2; ++i) {
		firstSum += static_cast<int>(i + 2) * allCodewords[i];
	}
	int firstCheck = firstSum % 107;

	// Calculate second check digit
	int secondSum = 0;
	for (size_t i = 0; i < allCodewords.size() - 2; ++i) {
		secondSum += static_cast<int>(i + 1) * allCodewords[i];
	}
	secondSum += firstCheck * static_cast<int>(allCodewords.size() - 1);
	int secondCheck = secondSum % 107;

	// Verify against actual check digits
	size_t n = allCodewords.size();
	return (allCodewords[n - 2] == firstCheck && allCodewords[n - 1] == secondCheck);
}

Barcode Code16KReader::decodeInternal(const BitMatrix& image, bool tryRotated) const
{
	int width = tryRotated ? image.height() : image.width();
	int height = tryRotated ? image.width() : image.height();

	if (width < 70 || height < 10) // Minimum size check
		return {};

	// Scan for rows
	std::vector<Code16KRow> rows;
	int lastRowY = -1;

	for (int y = 0; y < height; y++) {
		PatternRow bars;
		if (tryRotated) {
			// Read column as row
			bars.reserve(height / 2);
			bool lastBit = image.get(y, 0);
			int count = 1;
			for (int x = 1; x < height; ++x) {
				bool bit = image.get(y, x);
				if (bit == lastBit) {
					count++;
				} else {
					bars.push_back(count);
					count = 1;
					lastBit = bit;
				}
			}
			bars.push_back(count);
		} else {
			bars = GetPatternRow(image, y);
		}

		if (bars.empty())
			continue;

		Code16KRow row = DecodeRow(bars);
		if (row.isValid) {
			// Check if this is a new row (not too close to previous)
			if (lastRowY < 0 || y - lastRowY > 3) {
				// Check if we already have this row index
				bool duplicate = false;
				for (const auto& existingRow : rows) {
					if (existingRow.rowIndex == row.rowIndex) {
						duplicate = true;
						break;
					}
				}
				if (!duplicate) {
					rows.push_back(row);
					lastRowY = y;
				}
			}
		}
	}

	if (rows.size() < MIN_ROWS)
		return {};

	// Sort rows by row index
	std::sort(rows.begin(), rows.end(), [](const Code16KRow& a, const Code16KRow& b) {
		return a.rowIndex < b.rowIndex;
	});

	// Verify we have consecutive rows starting from 0
	for (size_t i = 0; i < rows.size(); ++i) {
		if (rows[i].rowIndex != static_cast<int>(i))
			return {};
	}

	// Extract row indicator from first codeword
	// Row indicator = (7 * (rows - 2)) + mode
	int firstCodeword = rows[0].codewords[0];
	int expectedRows = (firstCodeword / 7) + 2;
	int mode = firstCodeword % 7;

	if (expectedRows != static_cast<int>(rows.size()))
		return {};

	// Collect all codewords from all rows
	std::vector<int> allCodewords;
	for (const auto& row : rows) {
		for (int cw : row.codewords) {
			allCodewords.push_back(cw);
		}
	}

	// Validate check digits (last two codewords)
	if (!ValidateCheckDigits(allCodewords)) {
		// Continue anyway, might still be readable
	}

	// Determine initial code set from mode
	int codeSet = CODE_CODE_A;
	if (mode == 1 || mode == 5)
		codeSet = CODE_CODE_B;
	else if (mode == 2 || mode == 4 || mode == 6)
		codeSet = CODE_CODE_C;

	// Decode data (skip first codeword which is row indicator, and last two which are check digits)
	std::string result;
	bool fnc4Active = false;
	bool shift = false;

	for (size_t i = 1; i < allCodewords.size() - 2; ++i) {
		int code = allCodewords[i];

		// Skip padding characters (106 in Code 128/16K)
		if (code == 106)
			continue;

		DecodeCharacter(code, codeSet, result, fnc4Active, shift);
	}

	if (result.empty())
		return {};

	// Calculate position
	int xStart = 0;
	int xStop = width - 1;

	// Symbology identifier: ]K0 for Code 16K
	SymbologyIdentifier symbologyIdentifier = {'K', '0'};

	return Barcode(result, 0, xStart, xStop, BarcodeFormat::Code16K, symbologyIdentifier, Error());
}

Barcode Code16KReader::decode(const BinaryBitmap& image) const
{
	auto bits = image.getBitMatrix();
	if (!bits)
		return {};

	Barcode result = decodeInternal(*bits, false);

	if (!result.isValid() && _opts.tryRotate()) {
		result = decodeInternal(*bits, true);
	}

	return result;
}

Barcodes Code16KReader::decode(const BinaryBitmap& image, int maxSymbols) const
{
	Barcodes results;

	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}

	return results;
}

} // namespace ZXing::OneD
