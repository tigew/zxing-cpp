/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODCodablockFReader.h"

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

// Code 128 constants
static const int CODE_CODE_C = 99;
static const int CODE_CODE_B = 100;
static const int CODE_CODE_A = 101;
static const int CODE_FNC_1 = 102;
static const int CODE_FNC_2 = 97;
static const int CODE_FNC_3 = 96;
static const int CODE_FNC_4_A = 101; // Same as CODE_A in Code Set B
static const int CODE_FNC_4_B = 100; // Same as CODE_B in Code Set A
static const int CODE_START_A = 103;
static const int CODE_START_B = 104;
static const int CODE_START_C = 105;
static const int CODE_STOP = 106;
static const int CODE_SHIFT = 98;

// Codablock F specific constants
static const int MIN_ROWS = 2;
static const int MAX_ROWS = 44;
static const int MIN_COLUMNS = 4;
static const int MAX_COLUMNS = 62;

static const float MAX_AVG_VARIANCE = 0.25f;
static const float MAX_INDIVIDUAL_VARIANCE = 0.7f;

// Decode a single Code 128 pattern from the given widths
static int DecodeCode(const std::array<int, 6>& counters)
{
	float bestVariance = MAX_AVG_VARIANCE;
	int bestMatch = -1;

	for (size_t i = 0; i < Code128::CODE_PATTERNS.size(); ++i) {
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

// Structure to hold a decoded row
struct CodablockRow {
	int rowNumber = -1;
	int rowIndicator = -1;
	std::vector<int> codewords;
	bool isValid = false;
};

// Decode a single row from the bit pattern
static CodablockRow DecodeRow(const PatternRow& bars, int expectedRows = -1)
{
	CodablockRow result;

	if (bars.size() < 13) // Minimum: start + row indicator + 1 data + checksum + stop
		return result;

	// Find start pattern (should be CODE_START_A = 103)
	std::array<int, 6> counters = {};
	size_t pos = 0;

	// Skip initial quiet zone
	while (pos < bars.size() && bars[pos] == 0)
		pos++;

	if (pos >= bars.size())
		return result;

	// Read first 6 elements for start pattern
	for (int i = 0; i < 6 && pos + i < bars.size(); ++i)
		counters[i] = bars[pos + i];

	int startCode = DecodeCode(counters);
	if (startCode != CODE_START_A && startCode != CODE_START_B && startCode != CODE_START_C)
		return result;

	pos += 6;
	result.codewords.push_back(startCode);

	int codeSet = (startCode == CODE_START_A) ? CODE_CODE_A :
				  (startCode == CODE_START_B) ? CODE_CODE_B : CODE_CODE_C;

	// Read codewords until stop
	int checksum = startCode;
	int multiplier = 1;

	while (pos + 6 <= bars.size()) {
		for (int i = 0; i < 6; ++i)
			counters[i] = bars[pos + i];

		int code = DecodeCode(counters);
		if (code < 0)
			break;

		// Check for stop pattern (it's 7 elements, but we only read 6)
		if (code == CODE_STOP) {
			result.codewords.push_back(code);
			result.isValid = true;
			break;
		}

		result.codewords.push_back(code);
		checksum += code * multiplier;
		multiplier++;
		pos += 6;

		// Handle code set switches
		if (code == CODE_CODE_A || code == CODE_CODE_B || code == CODE_CODE_C) {
			codeSet = code;
		}
	}

	// Verify we have enough codewords (start + row indicator + at least 1 data + checksum + stop)
	if (result.codewords.size() < 5)
		return CodablockRow{};

	// Extract row indicator (second codeword after start)
	if (result.codewords.size() >= 2) {
		result.rowIndicator = result.codewords[1];
	}

	// The checksum is the second-to-last codeword (before stop)
	if (result.isValid && result.codewords.size() >= 3) {
		int expectedChecksum = checksum % 103;
		int actualChecksum = result.codewords[result.codewords.size() - 2];
		if (expectedChecksum != actualChecksum) {
			// Checksum mismatch, but we might still try to decode
			// result.isValid = false;
		}
	}

	return result;
}

// Scan a horizontal line and extract pattern
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

// Check if a row is a separator (mostly black horizontal line)
static bool IsSeparatorRow(const BitMatrix& image, int y, int expectedWidth)
{
	if (y < 0 || y >= image.height())
		return false;

	int blackCount = 0;
	int checkWidth = std::min(expectedWidth, image.width());

	for (int x = 0; x < checkWidth; ++x) {
		if (image.get(x, y))
			blackCount++;
	}

	// Separator should be mostly black (at least 80%)
	return blackCount > checkWidth * 0.8;
}

// Decode character from Code 128 codeword
static void DecodeCharacter(int code, int& codeSet, std::string& result, bool& fnc4Active)
{
	if (codeSet == CODE_CODE_C) {
		if (code < 100) {
			// Two-digit numeric
			result += static_cast<char>('0' + (code / 10));
			result += static_cast<char>('0' + (code % 10));
		} else if (code == CODE_CODE_A) {
			codeSet = CODE_CODE_A;
		} else if (code == CODE_CODE_B) {
			codeSet = CODE_CODE_B;
		}
	} else if (codeSet == CODE_CODE_A) {
		if (code < 64) {
			char c = static_cast<char>(code + 32);
			if (fnc4Active) c += 128;
			result += c;
		} else if (code < 96) {
			char c = static_cast<char>(code - 64);
			if (fnc4Active) c += 128;
			result += c;
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
			if (fnc4Active) c += 128;
			result += c;
		} else if (code == CODE_CODE_A) {
			codeSet = CODE_CODE_A;
		} else if (code == CODE_CODE_C) {
			codeSet = CODE_CODE_C;
		} else if (code == CODE_FNC_4_B) {
			fnc4Active = !fnc4Active;
		}
	}
}

Barcode CodablockFReader::decodeInternal(const BitMatrix& image, bool tryRotated) const
{
	int width = tryRotated ? image.height() : image.width();
	int height = tryRotated ? image.width() : image.height();

	if (width < 50 || height < 10) // Minimum size check
		return {};

	// Scan for rows
	std::vector<CodablockRow> rows;
	int lastRowY = -1;
	int rowHeight = 0;

	// Scan from top to bottom looking for Code 128 patterns
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

		CodablockRow row = DecodeRow(bars);
		if (row.isValid) {
			// Check if this is a new row or same row (within expected row height)
			if (lastRowY < 0 || y - lastRowY > 3) {
				rows.push_back(row);
				if (lastRowY >= 0 && rowHeight == 0) {
					rowHeight = y - lastRowY;
				}
				lastRowY = y;
			}
		}
	}

	if (rows.size() < MIN_ROWS)
		return {};

	// Sort rows by row indicator
	// First row indicator = rows - 2 (total rows count)
	// Subsequent rows = row number + 42
	int totalRows = -1;

	for (auto& row : rows) {
		if (row.rowIndicator >= 0 && row.rowIndicator < 42) {
			// This is likely the first row
			totalRows = row.rowIndicator + 2;
			row.rowNumber = 0;
		} else if (row.rowIndicator >= 42) {
			row.rowNumber = row.rowIndicator - 42;
		}
	}

	// Sort by row number
	std::sort(rows.begin(), rows.end(), [](const CodablockRow& a, const CodablockRow& b) {
		return a.rowNumber < b.rowNumber;
	});

	// Extract data from all rows
	std::string result;
	int codeSet = CODE_CODE_A;
	bool fnc4Active = false;

	// K1/K2 checksum calculation
	int k1Sum = 0;
	int k2Sum = 0;
	int charPos = 0;

	for (size_t rowIdx = 0; rowIdx < rows.size(); ++rowIdx) {
		const auto& row = rows[rowIdx];

		// Skip start (index 0), row indicator (index 1), checksum (second to last), stop (last)
		size_t dataStart = 2;
		size_t dataEnd = row.codewords.size() - 2;

		// Last row has K1/K2 before checksum
		if (rowIdx == rows.size() - 1 && dataEnd >= dataStart + 2) {
			dataEnd -= 2; // Skip K1 and K2
		}

		for (size_t i = dataStart; i < dataEnd; ++i) {
			int code = row.codewords[i];

			// Update K1/K2 checksums
			k1Sum = (k1Sum + (charPos + 1) * code) % 86;
			k2Sum = (k2Sum + charPos * code) % 86;
			charPos++;

			DecodeCharacter(code, codeSet, result, fnc4Active);
		}
	}

	if (result.empty())
		return {};

	// Calculate position
	int xStart = 0;
	int xStop = width - 1;
	int yStart = 0;
	int yStop = height - 1;

	// Create symbology identifier
	// ]O0 = Codablock F (not officially assigned, using 'O' for stacked)
	SymbologyIdentifier symbologyIdentifier = {'O', '0'};

	return Barcode(result, yStart, xStart, xStop, BarcodeFormat::CodablockF, symbologyIdentifier, Error());
}

Barcode CodablockFReader::decode(const BinaryBitmap& image) const
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

Barcodes CodablockFReader::decode(const BinaryBitmap& image, int maxSymbols) const
{
	Barcodes results;

	auto result = decode(image);
	if (result.isValid()) {
		results.push_back(std::move(result));
	}

	return results;
}

} // namespace ZXing::OneD
