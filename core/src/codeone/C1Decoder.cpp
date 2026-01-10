/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "C1Decoder.h"

#include "C1Version.h"
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

namespace ZXing::CodeOne {

// Code One encoding modes (similar to Data Matrix)
enum class Mode {
	ASCII,
	C40,
	TEXT,
	X12,
	EDIFACT,
	BASE256
};

// C40/Text/X12 character sets
static const char C40_BASIC[] = {
	' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

static const char TEXT_BASIC[] = {
	' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};

static const char X12_BASIC[] = {
	'\r', '*', '>', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

/**
 * Extract codewords from the bit matrix based on version.
 */
static ByteArray ExtractCodewords(const BitMatrix& bits, const Version& version)
{
	int width = bits.width();
	int height = bits.height();

	// Calculate total codewords
	int totalCodewords = version.dataCodewords + version.ecCodewords;
	ByteArray codewords(totalCodewords);

	int codewordIndex = 0;
	int bitIndex = 0;
	uint8_t currentByte = 0;

	// Code One uses 8-bit codewords
	// The data is arranged in the symbol after the finder pattern
	// Skip the leftmost column (finder pattern) and start reading data

	for (int row = 0; row < height && codewordIndex < totalCodewords; ++row) {
		// Skip the horizontal finder bars (rows 0 and height-1 in some versions)
		for (int col = 1; col < width && codewordIndex < totalCodewords; ++col) {
			// Read bit
			if (bits.get(col, row)) {
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
 * Code One uses RS over GF(128) for versions A-H.
 */
static bool CorrectErrors(ByteArray& codewords, const Version& version)
{
	if (version.ecCodewords == 0)
		return true;

	std::vector<int> codewordsInt(codewords.begin(), codewords.end());

	// Code One uses GF(128) for error correction
	if (!ReedSolomonDecode(GenericGF::MaxiCodeField64(), codewordsInt, version.ecCodewords))
		return false;

	for (size_t i = 0; i < codewords.size(); ++i) {
		codewords[i] = static_cast<uint8_t>(codewordsInt[i]);
	}

	return true;
}

/**
 * Decode C40/Text encoded data.
 */
static void DecodeC40Text(const ByteArray& codewords, int& pos, Content& result, bool isText)
{
	const char* basic = isText ? TEXT_BASIC : C40_BASIC;
	int shift = 0;

	while (pos + 1 < Size(codewords)) {
		int c1 = codewords[pos++];
		int c2 = codewords[pos++];

		if (c1 == 254) // Unlatch
			break;

		int value = (c1 << 8) | c2;
		int u1 = (value - 1) / 1600;
		int u2 = ((value - 1) / 40) % 40;
		int u3 = (value - 1) % 40;

		for (int u : {u1, u2, u3}) {
			if (shift == 0) {
				if (u < 3) {
					shift = u + 1;
				} else if (u < 40) {
					result.push_back(basic[u - 3]);
				}
			} else {
				// Handle shift characters
				if (shift == 1) { // Shift 1: punctuation
					result.push_back(u + 32);
				} else if (shift == 2) { // Shift 2: more punctuation
					result.push_back(u + 96);
				} else { // Shift 3: upper case for Text, symbols for C40
					result.push_back(isText ? (u + 64) : (u + 96));
				}
				shift = 0;
			}
		}
	}
}

/**
 * Decode X12 encoded data.
 */
static void DecodeX12(const ByteArray& codewords, int& pos, Content& result)
{
	while (pos + 1 < Size(codewords)) {
		int c1 = codewords[pos++];
		int c2 = codewords[pos++];

		if (c1 == 254) // Unlatch
			break;

		int value = (c1 << 8) | c2;
		int u1 = (value - 1) / 1600;
		int u2 = ((value - 1) / 40) % 40;
		int u3 = (value - 1) % 40;

		for (int u : {u1, u2, u3}) {
			if (u < Size(X12_BASIC)) {
				result.push_back(X12_BASIC[u]);
			}
		}
	}
}

/**
 * Decode EDIFACT encoded data.
 */
static void DecodeEDIFACT(const ByteArray& codewords, int& pos, Content& result)
{
	while (pos + 2 < Size(codewords)) {
		int c1 = codewords[pos++];
		int c2 = codewords[pos++];
		int c3 = codewords[pos++];

		// EDIFACT packs 4 6-bit values into 3 bytes
		int v1 = (c1 >> 2) & 0x3F;
		int v2 = ((c1 & 0x03) << 4) | ((c2 >> 4) & 0x0F);
		int v3 = ((c2 & 0x0F) << 2) | ((c3 >> 6) & 0x03);
		int v4 = c3 & 0x3F;

		for (int v : {v1, v2, v3, v4}) {
			if (v == 31) // Unlatch
				return;
			result.push_back(v + 32);
		}
	}
}

/**
 * Decode Base 256 encoded data.
 */
static void DecodeBase256(const ByteArray& codewords, int& pos, Content& result)
{
	if (pos >= Size(codewords))
		return;

	int length = codewords[pos++];
	if (length == 0) {
		// Length is in next codeword
		if (pos >= Size(codewords))
			return;
		length = codewords[pos++];
		if (length == 0)
			length = 256;
	}

	for (int i = 0; i < length && pos < Size(codewords); ++i) {
		result.push_back(codewords[pos++]);
	}
}

/**
 * Decode the codewords into a string.
 */
static Content DecodeData(const ByteArray& codewords, int dataCodewords)
{
	Content result;
	result.symbology = {'o', '4', 0}; // Code One symbology identifier

	Mode mode = Mode::ASCII;
	int pos = 0;

	while (pos < dataCodewords) {
		uint8_t c = codewords[pos];

		if (mode == Mode::ASCII) {
			if (c <= 128) {
				// ASCII character
				result.push_back(c - 1);
				pos++;
			} else if (c == 129) {
				// Pad
				pos++;
			} else if (c >= 130 && c <= 229) {
				// Two digits 00-99
				int digits = c - 130;
				result.push_back('0' + (digits / 10));
				result.push_back('0' + (digits % 10));
				pos++;
			} else if (c == 230) {
				// Latch to C40
				mode = Mode::C40;
				pos++;
			} else if (c == 231) {
				// Latch to Base 256
				mode = Mode::BASE256;
				pos++;
				DecodeBase256(codewords, pos, result);
				mode = Mode::ASCII;
			} else if (c == 235) {
				// Upper shift (next char + 128)
				pos++;
				if (pos < dataCodewords) {
					result.push_back(codewords[pos++] + 127);
				}
			} else if (c == 238) {
				// Latch to EDIFACT
				mode = Mode::EDIFACT;
				pos++;
			} else if (c == 239) {
				// Latch to Text
				mode = Mode::TEXT;
				pos++;
			} else if (c == 240) {
				// Latch to X12
				mode = Mode::X12;
				pos++;
			} else {
				pos++;
			}
		} else if (mode == Mode::C40) {
			DecodeC40Text(codewords, pos, result, false);
			mode = Mode::ASCII;
		} else if (mode == Mode::TEXT) {
			DecodeC40Text(codewords, pos, result, true);
			mode = Mode::ASCII;
		} else if (mode == Mode::X12) {
			DecodeX12(codewords, pos, result);
			mode = Mode::ASCII;
		} else if (mode == Mode::EDIFACT) {
			DecodeEDIFACT(codewords, pos, result);
			mode = Mode::ASCII;
		} else {
			pos++;
		}
	}

	return result;
}

DecoderResult Decode(const BitMatrix& bits)
{
	int width = bits.width();
	int height = bits.height();

	// Find matching version
	const Version* version = FindVersion(width, height);
	if (!version) {
		// Try to detect version from finder pattern
		return FormatError("Unknown Code One version");
	}

	// Extract codewords
	ByteArray codewords = ExtractCodewords(bits, *version);

	// Error correction
	if (!CorrectErrors(codewords, *version)) {
		return ChecksumError();
	}

	// Decode data
	Content content = DecodeData(codewords, version->dataCodewords);

	if (content.bytes.empty()) {
		return FormatError("Empty symbol");
	}

	return DecoderResult(std::move(content))
		.setEcLevel(std::string(1, version->name))
		.setVersionNumber(version->name - 'A' + 1);
}

} // namespace ZXing::CodeOne
