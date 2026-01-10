/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODLOGMARSReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <string>

namespace ZXing::OneD {

// LOGMARS uses identical encoding to Code 39
// Character set: 0-9, A-Z, -, ., space, $, /, +, %, and * (start/stop)
static constexpr char ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%*";

// Each character has 9 elements (5 bars + 4 spaces), with 3 wide and 6 narrow
// Bit pattern: 1 = wide, 0 = narrow
static constexpr std::array CHARACTER_ENCODINGS = {
	0x034, 0x121, 0x061, 0x160, 0x031, 0x130, 0x070, 0x025, 0x124, 0x064, // 0-9
	0x109, 0x049, 0x148, 0x019, 0x118, 0x058, 0x00D, 0x10C, 0x04C, 0x01C, // A-J
	0x103, 0x043, 0x142, 0x013, 0x112, 0x052, 0x007, 0x106, 0x046, 0x016, // K-T
	0x181, 0x0C1, 0x1C0, 0x091, 0x190, 0x0D0, 0x085, 0x184, 0x0C4, 0x0A8, // U-$
	0x0A2, 0x08A, 0x02A, 0x094 // /-% , *
};

static_assert(Size(ALPHABET) == Size(CHARACTER_ENCODINGS), "table size mismatch");

// Each character has 5 bars and 4 spaces = 9 elements
constexpr int CHAR_LEN = 9;

// Quiet zone scale factor (same as Code 39)
constexpr float QUIET_ZONE_SCALE = 1.f / 3;

/**
 * Check if character is start/stop symbol.
 */
static bool IsStartOrStop(char c)
{
	return c == '*';
}

/**
 * Calculate Modulo 43 check digit.
 * The check digit is the value such that the sum of all character values mod 43 equals 0.
 */
static int CalculateMod43Checksum(const std::string& data)
{
	int sum = 0;
	for (char c : data) {
		int idx = IndexOf(ALPHABET, c);
		if (idx < 0)
			return -1;
		sum += idx;
	}
	return sum % 43;
}

/**
 * Validate Mod 43 check digit.
 * Returns true if the last character is a valid check digit for the preceding data.
 */
static bool ValidateMod43Checksum(const std::string& dataWithCheck)
{
	if (dataWithCheck.length() < 2)
		return false;

	std::string data = dataWithCheck.substr(0, dataWithCheck.length() - 1);
	int expectedCheckIdx = CalculateMod43Checksum(data);
	if (expectedCheckIdx < 0)
		return false;

	char expectedCheck = ALPHABET[expectedCheckIdx];
	return dataWithCheck.back() == expectedCheck;
}

Barcode LOGMARSReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	// LOGMARS requires at least: start + 1 data char + check digit + stop = 4 characters
	constexpr int minCharCount = 4;

	// Start pattern uses narrow bars/spaces at specific positions
	constexpr auto START_PATTERN = FixedSparcePattern<CHAR_LEN, 6>{0, 2, 3, 5, 7, 8};

	// Find start pattern with quiet zone
	next = FindLeftGuard(next, minCharCount * CHAR_LEN, START_PATTERN, QUIET_ZONE_SCALE * 12);
	if (!next.isValid())
		return {};

	// Verify start character
	char startChar = DecodeNarrowWidePattern(next, CHARACTER_ENCODINGS, ALPHABET);
	if (!IsStartOrStop(startChar))
		return {};

	int xStart = next.pixelsInFront();
	int maxInterCharacterSpace = next.sum() / 2;

	std::string txt;
	txt.reserve(20);

	// Decode characters until stop pattern
	do {
		// Check remaining input width and inter-character space
		if (!next.skipSymbol() || !next.skipSingle(maxInterCharacterSpace))
			return {};

		char c = DecodeNarrowWidePattern(next, CHARACTER_ENCODINGS, ALPHABET);
		if (c == 0)
			return {};

		txt += c;
	} while (!IsStartOrStop(txt.back()));

	// Remove stop asterisk
	txt.pop_back();

	// Check minimum length (at least 1 data char + check digit)
	if (txt.length() < 2)
		return {};

	// Check quiet zone after stop
	if (!next.hasQuietZoneAfter(QUIET_ZONE_SCALE))
		return {};

	// LOGMARS REQUIRES valid Mod 43 check digit
	Error error;
	if (!ValidateMod43Checksum(txt)) {
		error = ChecksumError();
	}

	// Remove check digit from result (it's validated but not part of data)
	std::string result = txt.substr(0, txt.length() - 1);

	// Symbology identifier for LOGMARS: ]L0
	// L = LOGMARS symbology
	// 0 = check digit validated and transmitted
	SymbologyIdentifier symbologyIdentifier = {'L', '0'};

	int xStop = next.pixelsTillEnd();
	return Barcode(result, rowNumber, xStart, xStop, BarcodeFormat::LOGMARS, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
