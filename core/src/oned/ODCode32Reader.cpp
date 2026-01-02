/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODCode32Reader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <cstdint>

namespace ZXing::OneD {

// Code 32 uses the same character encodings as Code 39
// Each character consists of 5 bars and 4 spaces, 3 of which are wide
static constexpr std::array CHARACTER_ENCODINGS = {
	0x034, 0x121, 0x061, 0x160, 0x031, 0x130, 0x070, 0x025, 0x124, 0x064, // 0-9
	0x109, 0x049, 0x148, 0x019, 0x118, 0x058, 0x00D, 0x10C, 0x04C, 0x01C, // A-J
	0x103, 0x043, 0x142, 0x013, 0x112, 0x052, 0x007, 0x106, 0x046, 0x016, // K-T
	0x181, 0x0C1, 0x1C0, 0x091, 0x190, 0x0D0, 0x085, 0x184, 0x0C4, 0x0A8, // U-$
	0x0A2, 0x08A, 0x02A, 0x094 // /-% , *
};

// Standard Code 39 alphabet for pattern decoding
static constexpr char CODE39_ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%*";

// Code 32 Base-32 alphabet (excludes vowels A, E, I, O)
// Maps base-32 value (0-31) to character
static constexpr char BASE32_ALPHABET[] = "0123456789BCDFGHJKLMNPQRSTUVWXYZ";

// each character has 5 bars and 4 spaces
static constexpr int CHAR_LEN = 9;

/**
 * Convert a Code 32 base-32 character to its numeric value (0-31).
 * Returns -1 if the character is not a valid base-32 character.
 */
static int Base32Value(char c)
{
	for (int i = 0; i < 32; ++i) {
		if (BASE32_ALPHABET[i] == c)
			return i;
	}
	return -1;
}

/**
 * Convert 6 base-32 characters to a 32-bit integer.
 * Returns -1 if any character is invalid.
 */
static int64_t Base32ToNumber(const std::string& base32)
{
	if (base32.length() != 6)
		return -1;

	int64_t result = 0;
	for (char c : base32) {
		int value = Base32Value(c);
		if (value < 0)
			return -1;
		result = result * 32 + value;
	}
	return result;
}

/**
 * Convert a number to a 9-digit string, padded with leading zeros.
 */
static std::string NumberToDigits(int64_t num)
{
	if (num < 0 || num > 999999999)
		return {};

	std::string result(9, '0');
	for (int i = 8; i >= 0; --i) {
		result[i] = '0' + (num % 10);
		num /= 10;
	}
	return result;
}

/**
 * Calculate the Code 32 check digit using modulo 10 algorithm.
 *
 * Algorithm:
 * - For positions 0-7 (left to right):
 *   - Odd positions (1, 3, 5, 7): multiply digit by 2, subtract 9 if > 9
 *   - Even positions (0, 2, 4, 6): use digit as-is
 * - Sum all values, check digit = sum mod 10
 */
static int CalculateCheckDigit(const std::string& digits)
{
	if (digits.length() < 8)
		return -1;

	int sum = 0;
	for (int i = 0; i < 8; ++i) {
		int digit = digits[i] - '0';
		if (i % 2 == 1) { // Odd positions (1, 3, 5, 7)
			digit *= 2;
			if (digit > 9)
				digit -= 9;
		}
		sum += digit;
	}
	return sum % 10;
}

/**
 * Validate the check digit of a 9-digit Code 32 number.
 */
static bool ValidateCheckDigit(const std::string& digits)
{
	if (digits.length() != 9)
		return false;

	int expectedCheck = CalculateCheckDigit(digits);
	int actualCheck = digits[8] - '0';
	return expectedCheck == actualCheck;
}

Barcode Code32Reader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<RowReader::DecodingState>&) const
{
	// Code 32 is always 6 characters + start/stop = 8 characters
	constexpr int minCharCount = 8;
	auto isStartOrStopSymbol = [](char c) { return c == '*'; };

	// Same start pattern as Code 39
	constexpr auto START_PATTERN = FixedSparcePattern<CHAR_LEN, 6>{0, 2, 3, 5, 7, 8};
	constexpr float QUIET_ZONE_SCALE = 1.f / 3;

	next = FindLeftGuard(next, minCharCount * CHAR_LEN, START_PATTERN, QUIET_ZONE_SCALE * 12);
	if (!next.isValid())
		return {};

	// Read start pattern
	if (!isStartOrStopSymbol(DecodeNarrowWidePattern(next, CHARACTER_ENCODINGS, CODE39_ALPHABET)))
		return {};

	int xStart = next.pixelsInFront();
	int maxInterCharacterSpace = next.sum() / 2;

	std::string code39Text;
	code39Text.reserve(10);

	do {
		// Check remaining input width and inter-character space
		if (!next.skipSymbol() || !next.skipSingle(maxInterCharacterSpace))
			return {};

		char c = DecodeNarrowWidePattern(next, CHARACTER_ENCODINGS, CODE39_ALPHABET);
		if (c == 0)
			return {};

		code39Text += c;
	} while (!isStartOrStopSymbol(code39Text.back()));

	code39Text.pop_back(); // Remove stop asterisk

	// Check whitespace after the last char
	if (!next.hasQuietZoneAfter(QUIET_ZONE_SCALE))
		return {};

	// Code 32 must be exactly 6 characters
	if (code39Text.length() != 6)
		return {};

	// Verify all characters are valid base-32 characters
	for (char c : code39Text) {
		if (Base32Value(c) < 0)
			return {};
	}

	// Convert base-32 to 9-digit number
	int64_t numericValue = Base32ToNumber(code39Text);
	if (numericValue < 0 || numericValue > 999999999)
		return {};

	std::string digits = NumberToDigits(numericValue);
	if (digits.empty())
		return {};

	// Validate check digit
	bool hasValidCheckSum = ValidateCheckDigit(digits);
	Error error = !hasValidCheckSum ? ChecksumError() : Error();

	// Build human-readable output: 'A' prefix + 9 digits
	std::string txt = "A" + digits;

	// Symbology identifier: Code 32 uses 'A' with modifier '0' (no check digit validation reported)
	// Since Code 32 always has a check digit, we could use '1' to indicate validated
	SymbologyIdentifier symbologyIdentifier = {'A', hasValidCheckSum ? '1' : '0'};

	int xStop = next.pixelsTillEnd();
	return {std::move(txt), rowNumber, xStart, xStop, BarcodeFormat::Code32, symbologyIdentifier, error};
}

} // namespace ZXing::OneD
