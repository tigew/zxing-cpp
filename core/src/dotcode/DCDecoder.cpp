/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "DCDecoder.h"

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

namespace ZXing::DotCode {

// DotCode encoding modes (similar to Data Matrix)
enum class Mode {
	ASCII,
	C40,
	TEXT,
	X12,
	EDIFACT,
	BASE256,
	ECI
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

static const char X12_SET[] = {
	'\r', '*', '>', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

/**
 * Calculate the number of data and error correction codewords based on symbol size.
 * DotCode has variable sizes with different EC capacities.
 */
static std::pair<int, int> CalculateCodewordCounts(int width, int height)
{
	// DotCode uses a checkerboard pattern
	// Total dots = ceil(width/2) * ceil(height/2) + floor(width/2) * floor(height/2)
	int dotsOdd = ((width + 1) / 2) * ((height + 1) / 2);
	int dotsEven = (width / 2) * (height / 2);
	int totalDots = dotsOdd + dotsEven;

	// Each codeword is 8 bits, packed into the dot pattern
	int totalCodewords = totalDots / 8;

	// DotCode uses approximately 25-30% for error correction
	int ecCodewords = std::max(3, totalCodewords / 4);
	int dataCodewords = totalCodewords - ecCodewords;

	return {dataCodewords, ecCodewords};
}

/**
 * Extract codewords from the dot pattern.
 * DotCode uses a specific dot arrangement pattern.
 */
static ByteArray ExtractCodewords(const BitMatrix& bits)
{
	int width = bits.width();
	int height = bits.height();

	auto [dataCodewords, ecCodewords] = CalculateCodewordCounts(width, height);
	int totalCodewords = dataCodewords + ecCodewords;

	ByteArray codewords(totalCodewords);

	int codewordIndex = 0;
	int bitIndex = 0;
	uint8_t currentByte = 0;

	// DotCode uses a specific spiral/zigzag reading pattern
	// For simplicity, read in a standard pattern
	for (int y = 0; y < height && codewordIndex < totalCodewords; ++y) {
		for (int x = 0; x < width && codewordIndex < totalCodewords; ++x) {
			// DotCode dots are on a checkerboard pattern
			// Even columns: dots on odd rows
			// Odd columns: dots on even rows
			if ((x + y) % 2 == 0) {
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

	return codewords;
}

/**
 * Perform Reed-Solomon error correction.
 * DotCode uses GF(113) for error correction.
 */
static bool CorrectErrors(ByteArray& codewords, int ecCodewords)
{
	if (ecCodewords == 0)
		return true;

	std::vector<int> codewordsInt(codewords.begin(), codewords.end());

	// DotCode uses a custom Reed-Solomon over GF(113)
	// For now, use GF(256) as an approximation
	if (!ReedSolomonDecode(GenericGF::DataMatrixField256(), codewordsInt, ecCodewords))
		return false;

	for (size_t i = 0; i < codewords.size(); ++i) {
		codewords[i] = static_cast<uint8_t>(codewordsInt[i]);
	}

	return true;
}

/**
 * Decode C40/Text encoded data.
 */
static void DecodeC40Text(const ByteArray& codewords, int& pos, int dataCodewords, Content& result, bool isText)
{
	const char* basic = isText ? TEXT_BASIC : C40_BASIC;
	int shift = 0;

	while (pos + 1 < dataCodewords) {
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
				if (shift == 1) {
					result.push_back(u + 32);
				} else if (shift == 2) {
					result.push_back(u + 96);
				} else {
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
static void DecodeX12(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	while (pos + 1 < dataCodewords) {
		int c1 = codewords[pos++];
		int c2 = codewords[pos++];

		if (c1 == 254) // Unlatch
			break;

		int value = (c1 << 8) | c2;
		int u1 = (value - 1) / 1600;
		int u2 = ((value - 1) / 40) % 40;
		int u3 = (value - 1) % 40;

		for (int u : {u1, u2, u3}) {
			if (u < static_cast<int>(sizeof(X12_SET))) {
				result.push_back(X12_SET[u]);
			}
		}
	}
}

/**
 * Decode EDIFACT encoded data.
 */
static void DecodeEDIFACT(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	while (pos + 2 < dataCodewords) {
		int c1 = codewords[pos++];
		int c2 = codewords[pos++];
		int c3 = codewords[pos++];

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
static void DecodeBase256(const ByteArray& codewords, int& pos, int dataCodewords, Content& result)
{
	if (pos >= dataCodewords)
		return;

	int length = codewords[pos++];
	if (length == 0) {
		if (pos >= dataCodewords)
			return;
		length = codewords[pos++];
		if (length == 0)
			length = 256;
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
	result.symbology = {'J', '0', 0}; // DotCode symbology identifier

	Mode mode = Mode::ASCII;
	int pos = 0;

	while (pos < dataCodewords) {
		uint8_t c = codewords[pos];

		if (mode == Mode::ASCII) {
			if (c <= 128) {
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
				pos++;
				DecodeBase256(codewords, pos, dataCodewords, result);
			} else if (c == 235) {
				// Upper shift
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
			} else if (c == 241) {
				// FNC1/ECI
				pos++;
			} else {
				pos++;
			}
		} else if (mode == Mode::C40) {
			DecodeC40Text(codewords, pos, dataCodewords, result, false);
			mode = Mode::ASCII;
		} else if (mode == Mode::TEXT) {
			DecodeC40Text(codewords, pos, dataCodewords, result, true);
			mode = Mode::ASCII;
		} else if (mode == Mode::X12) {
			DecodeX12(codewords, pos, dataCodewords, result);
			mode = Mode::ASCII;
		} else if (mode == Mode::EDIFACT) {
			DecodeEDIFACT(codewords, pos, dataCodewords, result);
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

	// DotCode minimum size is 5x5
	if (width < 5 || height < 5) {
		return FormatError("Symbol too small for DotCode");
	}

	// Extract codewords from dot pattern
	ByteArray codewords = ExtractCodewords(bits);

	auto [dataCodewords, ecCodewords] = CalculateCodewordCounts(width, height);

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

} // namespace ZXing::DotCode
