/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODCode11Reader.h"

#include "ReaderOptions.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

#include <string>

namespace ZXing::OneD {

// Character set: 0-9, dash (-), start/stop (*)
static constexpr char ALPHABET[] = "0123456789-*";

// Each character has 3 bars and 2 spaces (5 elements).
// Bit pattern where 1 = wide, 0 = narrow (LSB = first element)
// Pattern from zint C11Table: bar, space, bar, space, bar
// 0: {1,1,1,1,2} = NNNNW = 0b10000 = 16
// 1: {2,1,1,1,2} = WNNNW = 0b10001 = 17
// 2: {1,2,1,1,2} = NWNNW = 0b10010 = 18
// 3: {2,2,1,1,1} = WWNNN = 0b00011 = 3
// 4: {1,1,2,1,2} = NNWNW = 0b10100 = 20
// 5: {2,1,2,1,1} = WNWNN = 0b00101 = 5
// 6: {1,2,2,1,1} = NWWNN = 0b00110 = 6
// 7: {1,1,1,2,2} = NNNWW = 0b11000 = 24
// 8: {2,1,1,2,1} = WNNWN = 0b01001 = 9
// 9: {2,1,1,1,1} = WNNNN = 0b00001 = 1
// -: {1,1,2,1,1} = NNWNN = 0b00100 = 4
// *: {1,1,2,2,1} = NNWWN = 0b01100 = 12
static constexpr int CHARACTER_ENCODINGS[] = {
	16,  // 0: NNNNW
	17,  // 1: WNNNW
	18,  // 2: NWNNW
	3,   // 3: WWNNN
	20,  // 4: NNWNW
	5,   // 5: WNWNN
	6,   // 6: NWWNN
	24,  // 7: NNNWW
	9,   // 8: WNNWN
	1,   // 9: WNNNN
	4,   // 10 (-): NNWNN
	12,  // 11 (*): NNWWN (start/stop)
};

static_assert(Size(ALPHABET) == Size(CHARACTER_ENCODINGS), "table size mismatch");

// Each character has 5 elements (3 bars + 2 spaces)
constexpr int CHAR_LEN = 5;
// Quiet zone should be at least 10 narrow elements according to spec
constexpr float QUIET_ZONE_SCALE = 0.5f;

// Start/stop pattern encoding
constexpr int START_STOP_PATTERN = 12; // 0b01100 = NNWWN

/**
 * Get the numeric value of a character for check digit calculation.
 * 0-9 = 0-9, dash = 10
 */
static int CharToValue(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c == '-')
		return 10;
	return -1;
}

/**
 * Calculate the C check digit (modulo 11 with weights 1-10).
 * @param data The data string (without check digits)
 * @return The C check digit value (0-10, where 10 = '-')
 */
static int CalculateCCheckDigit(const std::string& data)
{
	int sum = 0;
	int weight = 1;
	// Process from right to left
	for (int i = static_cast<int>(data.length()) - 1; i >= 0; --i) {
		int value = CharToValue(data[i]);
		if (value < 0)
			return -1;
		sum += value * weight;
		weight++;
		if (weight > 10)
			weight = 1;
	}
	return sum % 11;
}

/**
 * Calculate the K check digit (modulo 11 with weights 1-9).
 * This is calculated on data + C check digit.
 * @param data The data string including the C check digit
 * @return The K check digit value (0-10, where 10 = '-')
 */
static int CalculateKCheckDigit(const std::string& data)
{
	int sum = 0;
	int weight = 1;
	// Process from right to left
	for (int i = static_cast<int>(data.length()) - 1; i >= 0; --i) {
		int value = CharToValue(data[i]);
		if (value < 0)
			return -1;
		sum += value * weight;
		weight++;
		if (weight > 9)
			weight = 1;
	}
	return sum % 11;
}

/**
 * Convert a check digit value to its character representation.
 */
static char ValueToChar(int value)
{
	if (value >= 0 && value <= 9)
		return '0' + value;
	if (value == 10)
		return '-';
	return 0;
}

/**
 * Validate check digits in the decoded string.
 * @param txt The decoded string (without start/stop characters)
 * @return true if check digits are valid
 */
static bool ValidateCheckDigits(const std::string& txt)
{
	if (txt.length() < 2)
		return false;

	// Determine number of check digits based on data length
	// Data length <= 10: 1 check digit (C)
	// Data length > 10: 2 check digits (C and K)
	bool hasTwoCheckDigits = false;

	// Check if we might have two check digits
	if (txt.length() >= 3) {
		// Try with two check digits first
		std::string dataWithC = txt.substr(0, txt.length() - 1);
		std::string dataOnly = txt.substr(0, txt.length() - 2);

		if (dataOnly.length() > 10) {
			hasTwoCheckDigits = true;
		} else {
			// Could be either one or two check digits
			// Try two check digits first
			int expectedK = CalculateKCheckDigit(dataWithC);
			char expectedKChar = ValueToChar(expectedK);
			if (expectedKChar == txt.back()) {
				int expectedC = CalculateCCheckDigit(dataOnly);
				char expectedCChar = ValueToChar(expectedC);
				if (expectedCChar == dataWithC.back()) {
					hasTwoCheckDigits = true;
				}
			}
		}
	}

	if (hasTwoCheckDigits) {
		// Validate both C and K
		std::string dataWithC = txt.substr(0, txt.length() - 1);
		std::string dataOnly = txt.substr(0, txt.length() - 2);

		int expectedC = CalculateCCheckDigit(dataOnly);
		char expectedCChar = ValueToChar(expectedC);
		if (expectedCChar != dataWithC.back())
			return false;

		int expectedK = CalculateKCheckDigit(dataWithC);
		char expectedKChar = ValueToChar(expectedK);
		if (expectedKChar != txt.back())
			return false;
	} else {
		// Validate only C
		std::string dataOnly = txt.substr(0, txt.length() - 1);
		int expectedC = CalculateCCheckDigit(dataOnly);
		char expectedCChar = ValueToChar(expectedC);
		if (expectedCChar != txt.back())
			return false;
	}

	return true;
}

/**
 * Check if the pattern matches the start/stop character.
 */
static bool IsStartStopPattern(int pattern)
{
	return pattern == START_STOP_PATTERN;
}

static bool IsCode11LeftGuard(const PatternView& view, int spaceInPixel)
{
	return spaceInPixel > view.sum() * QUIET_ZONE_SCALE &&
		   IsStartStopPattern(RowReader::NarrowWideBitPattern(view));
}

Barcode Code11Reader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	// Minimum characters: start + at least 1 data + 1 check + stop = 4
	const int minCharCount = 4;

	next = FindLeftGuard<CHAR_LEN>(next, minCharCount * CHAR_LEN, IsCode11LeftGuard);
	if (!next.isValid())
		return {};

	int xStart = next.pixelsInFront();
	// Inter-character space should be narrow (about 1 unit)
	int maxInterCharacterSpace = next.sum() / 4;

	std::string txt;
	txt.reserve(20);

	// Verify start pattern
	int startPattern = NarrowWideBitPattern(next);
	if (!IsStartStopPattern(startPattern))
		return {};

	// Read characters until we hit the stop pattern
	while (true) {
		// Skip to next character
		if (!next.skipSymbol() || !next.skipSingle(maxInterCharacterSpace))
			return {};

		int pattern = NarrowWideBitPattern(next);
		if (pattern < 0)
			return {};

		// Check for stop pattern
		if (IsStartStopPattern(pattern))
			break;

		// Decode the character
		char c = LookupBitPattern(pattern, CHARACTER_ENCODINGS, ALPHABET);
		if (c == 0)
			return {};

		txt += c;
	}

	// Check minimum length and quiet zone after stop
	if (txt.length() < 2 || !next.hasQuietZoneAfter(QUIET_ZONE_SCALE))
		return {};

	// Validate check digits
	Error error;
	if (!ValidateCheckDigits(txt))
		error = ChecksumError();

	// Symbology identifier for Code 11
	// Modifier '0' = no check digit verification
	// Modifier '1' = one check digit verified and stripped
	// Modifier '2' = two check digits verified and stripped
	SymbologyIdentifier symbologyIdentifier = {'H', error ? '0' : '1'};

	int xStop = next.pixelsTillEnd();
	return Barcode(txt, rowNumber, xStart, xStop, BarcodeFormat::Code11, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
