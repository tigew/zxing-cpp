/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "HXDecoder.h"

#include "BitMatrix.h"
#include "ByteArray.h"
#include "Content.h"
#include "DecoderResult.h"
#include "GenericGF.h"
#include "ReedSolomonDecoder.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ZXing::HanXin {

// Han Xin version information
// Version 1-84, sizes 23x23 to 189x189 modules
// Size = (version * 2) + 21
struct Version {
	int version;
	int size;
	int dataCodewords[4]; // For each ECC level 1-4
};

// Han Xin encoding modes
enum class Mode {
	NUMERIC,        // Digits 0-9
	TEXT,           // Alphanumeric
	BINARY,         // Raw bytes
	REGION1,        // GB 18030 Chinese subset 1
	REGION2,        // GB 18030 Chinese subset 2
	DOUBLE_BYTE,    // GB 18030 2-byte
	FOUR_BYTE,      // GB 18030 4-byte
	ECI             // Extended Channel Interpretation
};

// Mode indicators (4 bits)
static const int MODE_NUMERIC = 0x1;
static const int MODE_TEXT = 0x2;
static const int MODE_BINARY_1 = 0x3;
static const int MODE_BINARY_2 = 0x4;
static const int MODE_REGION1 = 0x5;
static const int MODE_REGION2 = 0x6;
static const int MODE_DOUBLE_BYTE = 0x7;
static const int MODE_FOUR_BYTE = 0x8;
static const int MODE_ECI = 0x9;
static const int MODE_TERMINATOR = 0x0;

// Text mode subsets
static const char TEXT_SET1[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
static const char TEXT_SET2[] = "0123456789abcdefghijklmnopqrstuvwxyz ";

/**
 * Get version from symbol size.
 * Size = (version * 2) + 21, so version = (size - 21) / 2
 */
static int GetVersion(int size)
{
	if (size < 23 || size > 189)
		return 0;
	if ((size - 21) % 2 != 0)
		return 0;
	return (size - 21) / 2;
}

/**
 * Calculate the number of data codewords for a given version and ECC level.
 * Han Xin uses Reed-Solomon over GF(256) with polynomial 0x163.
 */
static int GetDataCodewords(int version, int eccLevel)
{
	if (version < 1 || version > 84 || eccLevel < 1 || eccLevel > 4)
		return 0;

	// Total codewords approximation based on version
	int size = version * 2 + 21;
	int modules = size * size;

	// Subtract finder patterns (4 corners, each 7x7 = 49, overlapping edge modules)
	// and alignment patterns
	int funcModules = 4 * 49 + (version > 1 ? (version / 7) * 25 : 0);
	int dataModules = modules - funcModules;

	// Each codeword is 8 bits
	int totalCodewords = dataModules / 8;

	// ECC percentages: L1=8%, L2=15%, L3=23%, L4=30%
	static const float eccPercent[] = {0.08f, 0.15f, 0.23f, 0.30f};
	int eccCodewords = static_cast<int>(totalCodewords * eccPercent[eccLevel - 1]);

	return totalCodewords - eccCodewords;
}

/**
 * Extract codewords from the symbol.
 */
static ByteArray ExtractCodewords(const BitMatrix& bits, int version)
{
	int size = bits.width();
	int dataCodewords = GetDataCodewords(version, 2); // Default to L2
	int eccCodewords = dataCodewords / 3; // Approximate
	int totalCodewords = dataCodewords + eccCodewords;

	ByteArray codewords(totalCodewords);
	int codewordIndex = 0;
	int bitIndex = 0;
	uint8_t currentByte = 0;

	// Han Xin uses a specific module placement pattern
	// Skip finder patterns at corners and alignment patterns
	for (int y = 0; y < size && codewordIndex < totalCodewords; ++y) {
		for (int x = 0; x < size && codewordIndex < totalCodewords; ++x) {
			// Skip finder pattern regions (corners)
			if ((x < 8 && y < 8) ||   // Top-left
				(x >= size - 8 && y < 8) ||   // Top-right
				(x < 8 && y >= size - 8) ||   // Bottom-left
				(x >= size - 8 && y >= size - 8))   // Bottom-right
				continue;

			if (bits.get(x, y)) {
				currentByte |= (1 << (7 - bitIndex));
			}
			bitIndex++;

			if (bitIndex == 8) {
				codewords[codewordIndex++] = currentByte;
				currentByte = 0;
				bitIndex = 0;
			}
		}
	}

	return codewords;
}

/**
 * Perform Reed-Solomon error correction.
 * Han Xin uses GF(256) with polynomial x^8 + x^6 + x^5 + x + 1 (0x163).
 */
static bool CorrectErrors(ByteArray& codewords, int eccCodewords)
{
	if (eccCodewords == 0)
		return true;

	std::vector<int> codewordsInt(codewords.begin(), codewords.end());

	// Use Data Matrix field (GF(256)) as approximation
	if (!ReedSolomonDecode(GenericGF::DataMatrixField256(), codewordsInt, eccCodewords))
		return false;

	for (size_t i = 0; i < codewords.size(); ++i) {
		codewords[i] = static_cast<uint8_t>(codewordsInt[i]);
	}

	return true;
}

/**
 * Decode numeric mode data.
 * 10 bits encode 3 digits.
 */
static void DecodeNumeric(const ByteArray& codewords, int& pos, int& bitPos, int count, Content& result)
{
	while (count > 0) {
		// Read 10 bits for 3 digits
		int value = 0;
		for (int i = 0; i < 10 && pos < static_cast<int>(codewords.size()); ++i) {
			value <<= 1;
			if (codewords[pos] & (1 << (7 - bitPos))) {
				value |= 1;
			}
			bitPos++;
			if (bitPos == 8) {
				bitPos = 0;
				pos++;
			}
		}

		if (count >= 3) {
			result.push_back('0' + (value / 100));
			result.push_back('0' + ((value / 10) % 10));
			result.push_back('0' + (value % 10));
			count -= 3;
		} else if (count == 2) {
			result.push_back('0' + (value / 10));
			result.push_back('0' + (value % 10));
			count = 0;
		} else {
			result.push_back('0' + value);
			count = 0;
		}
	}
}

/**
 * Decode text mode data.
 * 6 bits per character from subset.
 */
static void DecodeText(const ByteArray& codewords, int& pos, int& bitPos, int count, Content& result, int subset)
{
	const char* charset = (subset == 1) ? TEXT_SET1 : TEXT_SET2;

	while (count > 0 && pos < static_cast<int>(codewords.size())) {
		int value = 0;
		for (int i = 0; i < 6 && pos < static_cast<int>(codewords.size()); ++i) {
			value <<= 1;
			if (codewords[pos] & (1 << (7 - bitPos))) {
				value |= 1;
			}
			bitPos++;
			if (bitPos == 8) {
				bitPos = 0;
				pos++;
			}
		}

		if (value < 37) {
			result.push_back(charset[value]);
		}
		count--;
	}
}

/**
 * Decode binary mode data.
 * 8 bits per byte.
 */
static void DecodeBinary(const ByteArray& codewords, int& pos, int& bitPos, int count, Content& result)
{
	while (count > 0 && pos < static_cast<int>(codewords.size())) {
		int value = 0;
		for (int i = 0; i < 8 && pos < static_cast<int>(codewords.size()); ++i) {
			value <<= 1;
			if (codewords[pos] & (1 << (7 - bitPos))) {
				value |= 1;
			}
			bitPos++;
			if (bitPos == 8) {
				bitPos = 0;
				pos++;
			}
		}
		result.push_back(static_cast<uint8_t>(value));
		count--;
	}
}

/**
 * Decode Chinese region mode data.
 * 12 bits per character for GB 18030 subset.
 */
static void DecodeRegionChinese(const ByteArray& codewords, int& pos, int& bitPos, int count, Content& result, [[maybe_unused]] int region)
{
	while (count > 0 && pos < static_cast<int>(codewords.size())) {
		int value = 0;
		for (int i = 0; i < 12 && pos < static_cast<int>(codewords.size()); ++i) {
			value <<= 1;
			if (codewords[pos] & (1 << (7 - bitPos))) {
				value |= 1;
			}
			bitPos++;
			if (bitPos == 8) {
				bitPos = 0;
				pos++;
			}
		}

		// Convert to GB 18030 and store as 2 bytes
		int gb = value + 0xB0A1; // Approximate offset for region
		result.push_back(static_cast<uint8_t>((gb >> 8) & 0xFF));
		result.push_back(static_cast<uint8_t>(gb & 0xFF));
		count--;
	}
}

/**
 * Decode double-byte mode data.
 * 15 bits per character for GB 18030 2-byte encoding.
 */
static void DecodeDoubleByte(const ByteArray& codewords, int& pos, int& bitPos, int count, Content& result)
{
	while (count > 0 && pos < static_cast<int>(codewords.size())) {
		int value = 0;
		for (int i = 0; i < 15 && pos < static_cast<int>(codewords.size()); ++i) {
			value <<= 1;
			if (codewords[pos] & (1 << (7 - bitPos))) {
				value |= 1;
			}
			bitPos++;
			if (bitPos == 8) {
				bitPos = 0;
				pos++;
			}
		}

		// Convert to GB 18030 2-byte
		int byte1 = (value / 192) + 0x81;
		int byte2 = (value % 192) + 0x40;
		result.push_back(static_cast<uint8_t>(byte1));
		result.push_back(static_cast<uint8_t>(byte2));
		count--;
	}
}

/**
 * Decode four-byte mode data.
 * 21 bits per character for GB 18030 4-byte encoding.
 */
static void DecodeFourByte(const ByteArray& codewords, int& pos, int& bitPos, int count, Content& result)
{
	while (count > 0 && pos < static_cast<int>(codewords.size())) {
		int value = 0;
		for (int i = 0; i < 21 && pos < static_cast<int>(codewords.size()); ++i) {
			value <<= 1;
			if (codewords[pos] & (1 << (7 - bitPos))) {
				value |= 1;
			}
			bitPos++;
			if (bitPos == 8) {
				bitPos = 0;
				pos++;
			}
		}

		// Convert to GB 18030 4-byte
		int byte1 = (value / (10 * 126 * 10)) + 0x81;
		int temp = value % (10 * 126 * 10);
		int byte2 = (temp / (126 * 10)) + 0x30;
		temp = temp % (126 * 10);
		int byte3 = (temp / 10) + 0x81;
		int byte4 = (temp % 10) + 0x30;

		result.push_back(static_cast<uint8_t>(byte1));
		result.push_back(static_cast<uint8_t>(byte2));
		result.push_back(static_cast<uint8_t>(byte3));
		result.push_back(static_cast<uint8_t>(byte4));
		count--;
	}
}

/**
 * Read bits from codewords.
 */
static int ReadBits(const ByteArray& codewords, int& pos, int& bitPos, int numBits)
{
	int value = 0;
	for (int i = 0; i < numBits && pos < static_cast<int>(codewords.size()); ++i) {
		value <<= 1;
		if (codewords[pos] & (1 << (7 - bitPos))) {
			value |= 1;
		}
		bitPos++;
		if (bitPos == 8) {
			bitPos = 0;
			pos++;
		}
	}
	return value;
}

/**
 * Decode the codewords into content.
 */
static Content DecodeData(const ByteArray& codewords, int dataCodewords)
{
	Content result;
	result.symbology = {'H', 'X', 0}; // Han Xin symbology identifier

	int pos = 0;
	int bitPos = 0;

	while (pos < dataCodewords) {
		// Read 4-bit mode indicator
		int mode = ReadBits(codewords, pos, bitPos, 4);

		if (mode == MODE_TERMINATOR)
			break;

		// Read count indicator (varies by mode)
		int count = 0;
		switch (mode) {
			case MODE_NUMERIC:
				count = ReadBits(codewords, pos, bitPos, 13);
				DecodeNumeric(codewords, pos, bitPos, count, result);
				break;
			case MODE_TEXT:
				count = ReadBits(codewords, pos, bitPos, 13);
				DecodeText(codewords, pos, bitPos, count, result, 1);
				break;
			case MODE_BINARY_1:
			case MODE_BINARY_2:
				count = ReadBits(codewords, pos, bitPos, 13);
				DecodeBinary(codewords, pos, bitPos, count, result);
				break;
			case MODE_REGION1:
				count = ReadBits(codewords, pos, bitPos, 12);
				DecodeRegionChinese(codewords, pos, bitPos, count, result, 1);
				break;
			case MODE_REGION2:
				count = ReadBits(codewords, pos, bitPos, 12);
				DecodeRegionChinese(codewords, pos, bitPos, count, result, 2);
				break;
			case MODE_DOUBLE_BYTE:
				count = ReadBits(codewords, pos, bitPos, 12);
				DecodeDoubleByte(codewords, pos, bitPos, count, result);
				break;
			case MODE_FOUR_BYTE:
				count = ReadBits(codewords, pos, bitPos, 12);
				DecodeFourByte(codewords, pos, bitPos, count, result);
				break;
			case MODE_ECI:
				// Skip ECI designator
				ReadBits(codewords, pos, bitPos, 8);
				break;
			default:
				// Unknown mode, skip
				break;
		}
	}

	return result;
}

DecoderResult Decode(const BitMatrix& bits)
{
	int width = bits.width();
	int height = bits.height();

	// Han Xin must be square
	if (width != height) {
		return FormatError("Han Xin must be square");
	}

	// Check valid size range
	if (width < 23 || width > 189) {
		return FormatError("Invalid size for Han Xin");
	}

	// Detect version from size
	int version = GetVersion(width);
	if (version == 0) {
		return FormatError("Unknown Han Xin version");
	}

	// Extract codewords
	ByteArray codewords = ExtractCodewords(bits, version);
	if (codewords.empty()) {
		return FormatError("Failed to extract codewords");
	}

	int dataCodewords = GetDataCodewords(version, 2);
	int eccCodewords = static_cast<int>(codewords.size()) - dataCodewords;

	// Error correction
	if (eccCodewords > 0 && !CorrectErrors(codewords, eccCodewords)) {
		return ChecksumError();
	}

	// Decode data
	Content content = DecodeData(codewords, dataCodewords);

	if (content.bytes.empty()) {
		return FormatError("Empty symbol");
	}

	return DecoderResult(std::move(content));
}

} // namespace ZXing::HanXin
