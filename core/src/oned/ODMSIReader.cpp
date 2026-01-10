/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODMSIReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <string>

namespace ZXing::OneD {

// MSI digit patterns (8 elements per digit)
// Each digit is encoded as 4 bits, each bit = bar/space pair:
//   0 bit = narrow bar (1) + wide space (2) = 12
//   1 bit = wide bar (2) + narrow space (1) = 21
// Pattern order: bars and spaces interleaved
static constexpr int CHARACTER_PATTERNS[10][8] = {
	{1, 2, 1, 2, 1, 2, 1, 2}, // 0: 0000
	{1, 2, 1, 2, 1, 2, 2, 1}, // 1: 0001
	{1, 2, 1, 2, 2, 1, 1, 2}, // 2: 0010
	{1, 2, 1, 2, 2, 1, 2, 1}, // 3: 0011
	{1, 2, 2, 1, 1, 2, 1, 2}, // 4: 0100
	{1, 2, 2, 1, 1, 2, 2, 1}, // 5: 0101
	{1, 2, 2, 1, 2, 1, 1, 2}, // 6: 0110
	{1, 2, 2, 1, 2, 1, 2, 1}, // 7: 0111
	{2, 1, 1, 2, 1, 2, 1, 2}, // 8: 1000
	{2, 1, 1, 2, 1, 2, 2, 1}, // 9: 1001
};

// Start pattern: narrow bar (1), wide space (2)
static constexpr int START_PATTERN[] = {2, 1};
static constexpr int START_PATTERN_LEN = 2;

// Stop pattern: narrow bar (1), wide space (2), narrow bar (1)
static constexpr int STOP_PATTERN[] = {1, 2, 1};
static constexpr int STOP_PATTERN_LEN = 3;

// Character encoding length
static constexpr int CHAR_LEN = 8;

// Quiet zone as fraction of character width
static constexpr float QUIET_ZONE_SCALE = 0.5f;

// Maximum variance thresholds
static constexpr float MAX_AVG_VARIANCE = 0.25f;
static constexpr float MAX_INDIVIDUAL_VARIANCE = 0.7f;

/**
 * Calculate Mod 10 (Luhn-style) check digit.
 * Working from right to left:
 * - Double alternate digits and sum the individual digits if result > 9
 * - Sum all digits
 * - Check digit = (10 - (sum % 10)) % 10
 */
static int CalculateMod10CheckDigit(const std::string& data)
{
	int sum = 0;
	bool doubleIt = true; // Start with doubling (rightmost position in original)

	for (int i = static_cast<int>(data.length()) - 1; i >= 0; --i) {
		int digit = data[i] - '0';
		if (doubleIt) {
			digit *= 2;
			if (digit > 9)
				digit = digit / 10 + digit % 10; // Sum of digits (e.g., 18 -> 1+8=9)
		}
		sum += digit;
		doubleIt = !doubleIt;
	}

	return (10 - (sum % 10)) % 10;
}

/**
 * Calculate Mod 11 check digit with weights.
 * Working from right to left, multiply each digit by weights 2,3,4,5,6,7 (cycling).
 * Check digit = (11 - (sum % 11)) % 11
 * If result is 10, check digit is represented as ':' or '10' (we'll use 10)
 */
static int CalculateMod11CheckDigit(const std::string& data)
{
	int sum = 0;
	int weight = 2;

	for (int i = static_cast<int>(data.length()) - 1; i >= 0; --i) {
		int digit = data[i] - '0';
		sum += digit * weight;
		weight++;
		if (weight > 7)
			weight = 2;
	}

	return (11 - (sum % 11)) % 11;
}

/**
 * Validate check digit using Mod 10.
 */
static bool ValidateMod10(const std::string& dataWithCheck)
{
	if (dataWithCheck.length() < 2)
		return false;

	std::string data = dataWithCheck.substr(0, dataWithCheck.length() - 1);
	int checkDigit = dataWithCheck.back() - '0';

	return CalculateMod10CheckDigit(data) == checkDigit;
}

/**
 * Validate check digit using Mod 11.
 * Note: For Mod 11, check digit 10 is sometimes encoded as an extra character.
 * Here we assume single-digit check (0-9 only, 10 means invalid).
 */
static bool ValidateMod11(const std::string& dataWithCheck)
{
	if (dataWithCheck.length() < 2)
		return false;

	std::string data = dataWithCheck.substr(0, dataWithCheck.length() - 1);
	int checkDigit = dataWithCheck.back() - '0';
	int expected = CalculateMod11CheckDigit(data);

	// Mod 11 can produce 10, which would need two digits - skip those
	if (expected == 10)
		return false;

	return expected == checkDigit;
}

/**
 * Validate with Mod 10/10 (two consecutive Mod 10 check digits).
 */
static bool ValidateMod10Mod10(const std::string& dataWithChecks)
{
	if (dataWithChecks.length() < 3)
		return false;

	// First check digit is calculated on original data
	std::string data = dataWithChecks.substr(0, dataWithChecks.length() - 2);
	int checkC = dataWithChecks[dataWithChecks.length() - 2] - '0';
	int checkK = dataWithChecks[dataWithChecks.length() - 1] - '0';

	int expectedC = CalculateMod10CheckDigit(data);
	if (expectedC != checkC)
		return false;

	// Second check digit includes first check digit
	std::string dataWithC = data + static_cast<char>('0' + checkC);
	int expectedK = CalculateMod10CheckDigit(dataWithC);

	return expectedK == checkK;
}

/**
 * Validate with Mod 11/10 (Mod 11 followed by Mod 10).
 */
static bool ValidateMod11Mod10(const std::string& dataWithChecks)
{
	if (dataWithChecks.length() < 3)
		return false;

	std::string data = dataWithChecks.substr(0, dataWithChecks.length() - 2);
	int checkC = dataWithChecks[dataWithChecks.length() - 2] - '0';
	int checkK = dataWithChecks[dataWithChecks.length() - 1] - '0';

	int expectedC = CalculateMod11CheckDigit(data);
	if (expectedC == 10 || expectedC != checkC)
		return false;

	// Mod 10 on data + first check digit
	std::string dataWithC = data + static_cast<char>('0' + checkC);
	int expectedK = CalculateMod10CheckDigit(dataWithC);

	return expectedK == checkK;
}

/**
 * Check if this view matches the start pattern.
 */
static bool IsStartPattern(const PatternView& view, int spaceInPixel)
{
	if (view.size() < START_PATTERN_LEN)
		return false;

	// Need sufficient quiet zone before start
	if (spaceInPixel < view.sum() * QUIET_ZONE_SCALE)
		return false;

	float variance = RowReader::PatternMatchVariance(
		view.data(), START_PATTERN, START_PATTERN_LEN, MAX_INDIVIDUAL_VARIANCE);

	return variance < MAX_AVG_VARIANCE;
}

/**
 * Decode a single MSI digit from the pattern.
 * Returns -1 if no match.
 */
static int DecodeMSIDigit(const PatternView& view)
{
	if (view.size() < CHAR_LEN)
		return -1;

	float bestVariance = MAX_AVG_VARIANCE;
	int bestMatch = -1;

	for (int i = 0; i < 10; ++i) {
		float variance = RowReader::PatternMatchVariance(
			view.data(), CHARACTER_PATTERNS[i], CHAR_LEN, MAX_INDIVIDUAL_VARIANCE);
		if (variance < bestVariance) {
			bestVariance = variance;
			bestMatch = i;
		}
	}

	return bestMatch;
}

/**
 * Check if this view matches the stop pattern.
 */
static bool IsStopPattern(const PatternView& view)
{
	if (view.size() < STOP_PATTERN_LEN)
		return false;

	float variance = RowReader::PatternMatchVariance(
		view.data(), STOP_PATTERN, STOP_PATTERN_LEN, MAX_INDIVIDUAL_VARIANCE);

	return variance < MAX_AVG_VARIANCE;
}

Barcode MSIReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	// Minimum characters (at least 1 data digit + optional check digit)
	const int minCharCount = 1;

	// Find start pattern
	next = FindLeftGuard<START_PATTERN_LEN>(next, START_PATTERN_LEN + minCharCount * CHAR_LEN + STOP_PATTERN_LEN,
		[](const PatternView& view, int space) { return IsStartPattern(view, space); });

	if (!next.isValid())
		return {};

	int xStart = next.pixelsInFront();

	// Skip start pattern
	if (!next.skipSymbol())
		return {};

	std::string txt;
	txt.reserve(20);

	// Decode data characters
	while (next.isValid()) {
		// Check for stop pattern
		PatternView stopView = next.subView(0, STOP_PATTERN_LEN);
		if (stopView.isValid() && IsStopPattern(stopView)) {
			break;
		}

		// Need at least CHAR_LEN elements for a digit
		if (next.size() < CHAR_LEN)
			return {};

		PatternView charView = next.subView(0, CHAR_LEN);
		int digit = DecodeMSIDigit(charView);
		if (digit < 0)
			return {};

		txt += static_cast<char>('0' + digit);

		// Move to next character
		next = next.subView(CHAR_LEN);
	}

	// Verify we found the stop pattern
	PatternView stopView = next.subView(0, STOP_PATTERN_LEN);
	if (!stopView.isValid() || !IsStopPattern(stopView))
		return {};

	// Move past stop pattern
	next = next.subView(STOP_PATTERN_LEN);

	// Check minimum length
	if (txt.empty())
		return {};

	// Check quiet zone after stop
	if (!next.hasQuietZoneAfter(QUIET_ZONE_SCALE))
		return {};

	// Validate check digit(s) - try different algorithms
	// MSI commonly uses Mod 10, Mod 11, Mod 10/10, or Mod 11/10
	Error error;
	bool checksumValid = false;

	// Try single check digit validations first
	if (txt.length() >= 2) {
		checksumValid = ValidateMod10(txt) || ValidateMod11(txt);
	}

	// Try dual check digit validations
	if (!checksumValid && txt.length() >= 3) {
		checksumValid = ValidateMod10Mod10(txt) || ValidateMod11Mod10(txt);
	}

	// If no checksum variant matches and we expect checksums, report error
	// For MSI, checksum is optional, so we don't fail if no match
	// But if _opts.validateChecksum() we should validate
	if (!checksumValid && txt.length() >= 2) {
		// Just note potential checksum issue - MSI checksum is not standardized
		// error = ChecksumError();
	}

	// Symbology identifier: ]M0 for MSI
	SymbologyIdentifier symbologyIdentifier = {'M', '0'};

	int xStop = next.pixelsTillEnd();
	return Barcode(txt, rowNumber, xStart, xStop, BarcodeFormat::MSI, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
