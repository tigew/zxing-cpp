/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODPZNReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <cstdint>

namespace ZXing::OneD {

// Code 39 character encodings (same as used by Code 39, LOGMARS, Code 32)
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

// Each character has 5 bars and 4 spaces
static constexpr int CHAR_LEN = 9;

/**
 * Calculate the PZN check digit using Modulo 11 algorithm.
 *
 * For PZN7 (6 data digits): weights are 2, 3, 4, 5, 6, 7
 * For PZN8 (7 data digits): weights are 1, 2, 3, 4, 5, 6, 7
 *
 * Algorithm:
 * 1. Multiply each digit by its weight (position + offset)
 * 2. Sum all products
 * 3. Take sum mod 11
 * 4. If result is 10, the PZN is invalid
 *
 * @param digits The data digits (without check digit)
 * @return The check digit (0-9), or -1 if invalid (would be 10)
 */
static int CalculatePZNCheckDigit(const std::string& digits)
{
	int sum = 0;
	int numDigits = static_cast<int>(digits.length());

	// Weight offset: PZN7 uses weights 2-7, PZN8 uses weights 1-7
	// For 6 digits: offset = 2, weights = 2,3,4,5,6,7
	// For 7 digits: offset = 1, weights = 1,2,3,4,5,6,7
	int weightOffset = (numDigits == 6) ? 2 : 1;

	for (int i = 0; i < numDigits; ++i) {
		if (digits[i] < '0' || digits[i] > '9')
			return -1;
		int digit = digits[i] - '0';
		int weight = i + weightOffset;
		sum += digit * weight;
	}

	int checkDigit = sum % 11;

	// If check digit is 10, the PZN is invalid and not assigned
	if (checkDigit == 10)
		return -1;

	return checkDigit;
}

/**
 * Validate the check digit of a PZN number.
 * @param pzn The complete PZN (data digits + check digit)
 * @return true if the check digit is valid
 */
static bool ValidatePZNCheckDigit(const std::string& pzn)
{
	if (pzn.length() != 7 && pzn.length() != 8)
		return false;

	// Split into data and check digit
	std::string dataDigits = pzn.substr(0, pzn.length() - 1);
	int expectedCheck = CalculatePZNCheckDigit(dataDigits);

	if (expectedCheck < 0)
		return false;

	int actualCheck = pzn.back() - '0';
	return expectedCheck == actualCheck;
}

Barcode PZNReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<RowReader::DecodingState>&) const
{
	// PZN format: *-XXXXXXXY* where X are data digits and Y is check digit
	// Minimum: start(*) + minus(-) + 6 digits + check + stop(*) = 10 characters for PZN7
	// Maximum: start(*) + minus(-) + 7 digits + check + stop(*) = 11 characters for PZN8
	constexpr int minCharCount = 10;
	auto isStartOrStopSymbol = [](char c) { return c == '*'; };

	// Same start pattern as Code 39
	constexpr auto START_PATTERN = FixedSparcePattern<CHAR_LEN, 6>{0, 2, 3, 5, 7, 8};
	constexpr float QUIET_ZONE_SCALE = 1.f / 3;

	next = FindLeftGuard(next, minCharCount * CHAR_LEN, START_PATTERN, QUIET_ZONE_SCALE * 12);
	if (!next.isValid())
		return {};

	// Read start pattern - must be asterisk
	if (!isStartOrStopSymbol(DecodeNarrowWidePattern(next, CHARACTER_ENCODINGS, CODE39_ALPHABET)))
		return {};

	int xStart = next.pixelsInFront();
	int maxInterCharacterSpace = next.sum() / 2;

	std::string code39Text;
	code39Text.reserve(12);

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

	// PZN must start with minus sign (the PZN identifier)
	if (code39Text.empty() || code39Text[0] != '-')
		return {};

	// Extract the numeric part (after the minus sign)
	std::string pznDigits = code39Text.substr(1);

	// Must be 7 digits (PZN7) or 8 digits (PZN8)
	if (pznDigits.length() != 7 && pznDigits.length() != 8)
		return {};

	// Verify all characters are digits
	for (char c : pznDigits) {
		if (c < '0' || c > '9')
			return {};
	}

	// Validate check digit
	bool hasValidCheckSum = ValidatePZNCheckDigit(pznDigits);
	Error error = !hasValidCheckSum ? ChecksumError() : Error();

	// Build human-readable output: "PZN-" prefix + digits
	// The standard display format is "PZN-XXXXXXXY" or "PZN XXXXXXXY"
	std::string txt = "PZN-" + pznDigits;

	// Symbology identifier for Code 39 variants
	// Using 'A' for Code 39 family, modifier indicates check digit status
	SymbologyIdentifier symbologyIdentifier = {'A', hasValidCheckSum ? '1' : '0'};

	int xStop = next.pixelsTillEnd();
	return {std::move(txt), rowNumber, xStart, xStop, BarcodeFormat::PZN, symbologyIdentifier, error};
}

} // namespace ZXing::OneD
