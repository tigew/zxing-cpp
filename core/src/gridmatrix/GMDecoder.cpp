/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "GMDecoder.h"

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

namespace ZXing::GridMatrix {

// Grid Matrix version information
// Version 1-13, sizes 18x18 to 162x162 modules (in 12-module increments)
struct Version {
	int version;
	int size;           // Symbol size in modules
	int dataRegions;    // Number of data regions (macromodule grid size)
};

static const Version VERSIONS[] = {
	{1, 18, 3},
	{2, 30, 5},
	{3, 42, 7},
	{4, 54, 9},
	{5, 66, 11},
	{6, 78, 13},
	{7, 90, 15},
	{8, 102, 17},
	{9, 114, 19},
	{10, 126, 21},
	{11, 138, 23},
	{12, 150, 25},
	{13, 162, 27},
};

// Grid Matrix encoding modes
enum class Mode {
	NUMERIC,      // Digits 0-9
	UPPERCASE,    // Uppercase letters A-Z, space
	MIXED,        // Alphanumeric and punctuation
	CHINESE,      // Chinese characters (GB 2312)
	BINARY,       // Raw bytes
	ECI           // Extended Channel Interpretation
};

// Error correction levels (L1-L5)
// L1: lowest, L5: highest
static const float EC_LEVELS[] = {0.10f, 0.15f, 0.23f, 0.30f, 0.40f};

/**
 * Get version information from symbol size.
 */
static const Version* GetVersion(int size)
{
	for (const auto& v : VERSIONS) {
		if (v.size == size)
			return &v;
	}
	return nullptr;
}

/**
 * Calculate the number of data and error correction codewords.
 * Grid Matrix uses Reed-Solomon over GF(929).
 */
static std::pair<int, int> CalculateCodewordCounts(int version, int ecLevel)
{
	const Version* v = nullptr;
	for (const auto& ver : VERSIONS) {
		if (ver.version == version) {
			v = &ver;
			break;
		}
	}
	if (!v)
		return {0, 0};

	// Each macromodule (6x6) contains 24 data bits (3 bytes)
	// Total macromodules = (dataRegions)^2
	int totalMacromodules = v->dataRegions * v->dataRegions;
	// Subtract finder pattern and alignment pattern regions
	int dataMacromodules = totalMacromodules - 9; // 9 for finder patterns (corners + center)

	int totalBytes = dataMacromodules * 3;

	// Error correction codewords based on level
	float ecRatio = (ecLevel >= 0 && ecLevel < 5) ? EC_LEVELS[ecLevel] : EC_LEVELS[2];
	int ecCodewords = static_cast<int>(totalBytes * ecRatio);
	int dataCodewords = totalBytes - ecCodewords;

	return {dataCodewords, ecCodewords};
}

/**
 * Extract codewords from the symbol.
 * Grid Matrix uses a specific macromodule arrangement.
 */
static ByteArray ExtractCodewords(const BitMatrix& bits, int version)
{
	const Version* v = nullptr;
	for (const auto& ver : VERSIONS) {
		if (ver.version == version) {
			v = &ver;
			break;
		}
	}
	if (!v)
		return {};

	int size = v->size;
	int macroSize = 6; // Each macromodule is 6x6

	auto [dataCodewords, ecCodewords] = CalculateCodewordCounts(version, 2);
	int totalCodewords = dataCodewords + ecCodewords;

	ByteArray codewords(totalCodewords);
	int codewordIndex = 0;
	int bitIndex = 0;
	uint8_t currentByte = 0;

	// Read macromodules in a specific order, skipping finder patterns
	for (int my = 0; my < v->dataRegions && codewordIndex < totalCodewords; ++my) {
		for (int mx = 0; mx < v->dataRegions && codewordIndex < totalCodewords; ++mx) {
			// Skip finder pattern positions (corners and center)
			bool isFinderPosition =
				(mx < 2 && my < 2) ||  // Top-left
				(mx >= v->dataRegions - 2 && my < 2) ||  // Top-right
				(mx < 2 && my >= v->dataRegions - 2) ||  // Bottom-left
				(mx >= v->dataRegions - 2 && my >= v->dataRegions - 2) ||  // Bottom-right
				(mx == v->dataRegions / 2 && my == v->dataRegions / 2);  // Center

			if (isFinderPosition)
				continue;

			// Read bits from this macromodule
			int baseX = mx * macroSize;
			int baseY = my * macroSize;

			for (int dy = 0; dy < macroSize && codewordIndex < totalCodewords; ++dy) {
				for (int dx = 0; dx < macroSize && codewordIndex < totalCodewords; ++dx) {
					int x = baseX + dx;
					int y = baseY + dy;

					if (x < size && y < size) {
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
			}
		}
	}

	return codewords;
}

/**
 * Perform Reed-Solomon error correction.
 */
static bool CorrectErrors(ByteArray& codewords, int ecCodewords)
{
	if (ecCodewords == 0)
		return true;

	std::vector<int> codewordsInt(codewords.begin(), codewords.end());

	// Grid Matrix uses GF(929), but we approximate with GF(256)
	if (!ReedSolomonDecode(GenericGF::DataMatrixField256(), codewordsInt, ecCodewords))
		return false;

	for (size_t i = 0; i < codewords.size(); ++i) {
		codewords[i] = static_cast<uint8_t>(codewordsInt[i]);
	}

	return true;
}

/**
 * Decode numeric mode data.
 */
static void DecodeNumeric(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	// Numeric mode: packs 3 digits per 10 bits
	while (pos + 1 < dataCodewords) {
		int c1 = codewords[pos++];
		int c2 = (pos < dataCodewords) ? codewords[pos++] : 0;

		// Unpack digits
		int value = (c1 << 2) | ((c2 >> 6) & 0x03);
		if (value < 1000) {
			result.push_back('0' + (value / 100));
			result.push_back('0' + ((value / 10) % 10));
			result.push_back('0' + (value % 10));
		}

		// Check for mode switch
		if ((c2 & 0x3F) == 0x3F)
			break;
	}
}

/**
 * Decode uppercase mode data.
 */
static void DecodeUppercase(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	while (pos < dataCodewords) {
		uint8_t c = codewords[pos++];

		if (c == 0xFF) // Mode switch
			break;

		if (c < 26) {
			result.push_back('A' + c);
		} else if (c == 26) {
			result.push_back(' ');
		}
	}
}

/**
 * Decode mixed mode data.
 */
static void DecodeMixed(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	static const char MIXED_CHARS[] =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

	while (pos < dataCodewords) {
		uint8_t c = codewords[pos++];

		if (c == 0xFF) // Mode switch
			break;

		if (c < sizeof(MIXED_CHARS) - 1) {
			result.push_back(MIXED_CHARS[c]);
		}
	}
}

/**
 * Decode Chinese mode data (GB 2312).
 */
static void DecodeChinese(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	// Chinese mode uses 13 bits per character (GB 2312)
	while (pos + 1 < dataCodewords) {
		int c1 = codewords[pos++];
		int c2 = codewords[pos++];

		if (c1 == 0xFF && c2 == 0xFF) // Mode switch
			break;

		// Convert to GB 2312 code point
		int gb = (c1 << 8) | c2;

		// GB 2312 range: 0xA1A1 - 0xFEFE
		if (gb >= 0xA1A1) {
			// Store as two bytes (GB 2312 encoding)
			result.push_back(static_cast<uint8_t>((gb >> 8) & 0xFF));
			result.push_back(static_cast<uint8_t>(gb & 0xFF));
		}
	}
}

/**
 * Decode binary mode data.
 */
static void DecodeBinary(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	if (pos >= dataCodewords)
		return;

	// First byte is length
	int length = codewords[pos++];
	if (length == 0 && pos < dataCodewords) {
		// Extended length
		int len1 = codewords[pos++];
		int len2 = (pos < dataCodewords) ? codewords[pos++] : 0;
		length = (len1 << 8) | len2;
	}

	for (int i = 0; i < length && pos < dataCodewords; ++i) {
		result.push_back(codewords[pos++]);
	}
}

/**
 * Decode the codewords into content.
 */
static Content DecodeData(const ByteArray& codewords, int dataCodewords)
{
	Content result;
	result.symbology = {'G', 'M', 0}; // Grid Matrix symbology identifier

	Mode mode = Mode::MIXED; // Default mode
	int pos = 0;

	while (pos < dataCodewords) {
		uint8_t c = codewords[pos];

		// Check for mode latch codes
		if (c >= 0xF0 && c <= 0xF5) {
			pos++;
			switch (c) {
				case 0xF0: mode = Mode::NUMERIC; break;
				case 0xF1: mode = Mode::UPPERCASE; break;
				case 0xF2: mode = Mode::MIXED; break;
				case 0xF3: mode = Mode::CHINESE; break;
				case 0xF4: mode = Mode::BINARY; break;
				case 0xF5: mode = Mode::ECI; break;
			}
			continue;
		}

		// Decode based on current mode
		switch (mode) {
			case Mode::NUMERIC:
				DecodeNumeric(codewords, pos, dataCodewords, result);
				mode = Mode::MIXED;
				break;
			case Mode::UPPERCASE:
				DecodeUppercase(codewords, pos, dataCodewords, result);
				mode = Mode::MIXED;
				break;
			case Mode::MIXED:
				DecodeMixed(codewords, pos, dataCodewords, result);
				break;
			case Mode::CHINESE:
				DecodeChinese(codewords, pos, dataCodewords, result);
				mode = Mode::MIXED;
				break;
			case Mode::BINARY:
				DecodeBinary(codewords, pos, dataCodewords, result);
				mode = Mode::MIXED;
				break;
			case Mode::ECI:
				// Skip ECI designator
				if (pos < dataCodewords)
					pos++;
				mode = Mode::MIXED;
				break;
			default:
				pos++;
				break;
		}
	}

	return result;
}

/**
 * Detect the version from symbol size.
 */
static int DetectVersion(int size)
{
	for (const auto& v : VERSIONS) {
		if (v.size == size)
			return v.version;
	}
	return 0;
}

DecoderResult Decode(const BitMatrix& bits)
{
	int width = bits.width();
	int height = bits.height();

	// Grid Matrix must be square
	if (width != height) {
		return FormatError("Grid Matrix must be square");
	}

	// Minimum size is 18x18 (Version 1)
	if (width < 18) {
		return FormatError("Symbol too small for Grid Matrix");
	}

	// Detect version
	int version = DetectVersion(width);
	if (version == 0) {
		return FormatError("Unknown Grid Matrix version");
	}

	// Extract codewords
	ByteArray codewords = ExtractCodewords(bits, version);
	if (codewords.empty()) {
		return FormatError("Failed to extract codewords");
	}

	auto [dataCodewords, ecCodewords] = CalculateCodewordCounts(version, 2);

	// Error correction
	if (!CorrectErrors(codewords, ecCodewords)) {
		return ChecksumError();
	}

	// Decode data
	Content content = DecodeData(codewords, dataCodewords);

	if (content.bytes.empty()) {
		return FormatError("Empty symbol");
	}

	return DecoderResult(std::move(content));
}

} // namespace ZXing::GridMatrix
