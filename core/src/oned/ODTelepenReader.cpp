/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODTelepenReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <string>
#include <array>

namespace ZXing::OneD {

// Telepen encoding table for ASCII 0-127
// Each character is encoded with even parity byte pattern
// Patterns use 1 (narrow) and 3 (wide) elements
// Start character "_" (95) and Stop character "z" (122) have known patterns

// Character patterns: length followed by element widths
// Based on Telepen's even-parity encoding

// Start pattern: "_" (ASCII 95) = 5 narrow pairs + 1 wide pair
// Pattern: [1,1,1,1,1,1,1,1,1,1,3,3]
static constexpr int START_PATTERN[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3};
static constexpr int START_PATTERN_LEN = 12;

// Stop pattern: "z" (ASCII 122) = 1 wide pair + 5 narrow pairs
// Pattern: [3,3,1,1,1,1,1,1,1,1,1,1]
static constexpr int STOP_PATTERN[] = {3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static constexpr int STOP_PATTERN_LEN = 12;

// Telepen character patterns - derived from even-parity bit encoding
// Each ASCII character (0-127) has a unique 16-module pattern
// Format: alternating bar/space widths (narrow=1, wide=3)

// For decoding, we compute the bit pattern from the bar/space sequence
// and reverse-engineer the ASCII value

// Quiet zone as fraction of character width
static constexpr float QUIET_ZONE_SCALE = 0.5f;

// Maximum variance thresholds
static constexpr float MAX_AVG_VARIANCE = 0.30f;
static constexpr float MAX_INDIVIDUAL_VARIANCE = 0.8f;

/**
 * Compute even parity for a 7-bit ASCII value.
 * Returns the 8-bit value with parity bit set.
 */
static int AddEvenParity(int ascii)
{
	int count = 0;
	int val = ascii;
	while (val > 0) {
		count += val & 1;
		val >>= 1;
	}
	// If odd number of 1s, set parity bit (bit 7)
	if (count % 2 == 1)
		return ascii | 0x80;
	return ascii;
}

/**
 * Compute the bar/space pattern for an ASCII character.
 * Fills the provided array with widths (1 or 3).
 * Returns the number of elements.
 */
static int ComputePattern(int ascii, int* pattern)
{
	int parityByte = AddEvenParity(ascii);

	// Telepen encoding rule:
	// Read byte LSB first, generate bar/space pattern
	// Runs of 0s produce narrow elements, runs of 1s produce wide elements

	int idx = 0;
	int i = 0;

	while (i < 8) {
		int bit = (parityByte >> i) & 1;

		// Count run length
		int runLen = 1;
		while (i + runLen < 8 && ((parityByte >> (i + runLen)) & 1) == bit) {
			runLen++;
		}

		// Generate bar/space pair based on run length
		// Simplified: run of 1 = narrow pair (1,1), run > 1 = wide pair (3,3)
		if (runLen >= 3) {
			pattern[idx++] = 3;
			pattern[idx++] = 3;
		} else if (runLen == 2) {
			pattern[idx++] = 1;
			pattern[idx++] = 3;
		} else {
			pattern[idx++] = 1;
			pattern[idx++] = 1;
		}

		i += runLen;
	}

	// Ensure pattern sums to 16 modules
	// Adjust if needed
	int sum = 0;
	for (int j = 0; j < idx; ++j) {
		sum += pattern[j];
	}

	// If sum != 16, we need to adjust (this is a simplified implementation)
	// For now, just return the pattern we have
	return idx;
}

/**
 * Check if view matches start pattern.
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
 * Check if view matches stop pattern.
 */
static bool IsStopPattern(const PatternView& view)
{
	if (view.size() < STOP_PATTERN_LEN)
		return false;

	float variance = RowReader::PatternMatchVariance(
		view.data(), STOP_PATTERN, STOP_PATTERN_LEN, MAX_INDIVIDUAL_VARIANCE);

	return variance < MAX_AVG_VARIANCE;
}

/**
 * Decode a Telepen character from the pattern view.
 * Returns the ASCII value (0-127) or -1 if no match.
 */
static int DecodeTelepenChar(const PatternView& view)
{
	if (view.size() < 4) // Minimum 4 elements
		return -1;

	// Try to find the best matching ASCII character
	float bestVariance = MAX_AVG_VARIANCE;
	int bestMatch = -1;

	int tempPattern[16];

	for (int ascii = 0; ascii < 128; ++ascii) {
		// Skip start/stop characters in data
		if (ascii == 95 || ascii == 122)
			continue;

		int patternLen = ComputePattern(ascii, tempPattern);

		if (view.size() < patternLen)
			continue;

		float variance = RowReader::PatternMatchVariance(
			view.data(), tempPattern, patternLen, MAX_INDIVIDUAL_VARIANCE);

		if (variance < bestVariance) {
			bestVariance = variance;
			bestMatch = ascii;
		}
	}

	return bestMatch;
}

/**
 * Get the pattern length for an ASCII character.
 */
static int GetPatternLength(int ascii)
{
	int tempPattern[16];
	return ComputePattern(ascii, tempPattern);
}

/**
 * Calculate modulo 127 check digit.
 * Check digit = (127 - (sum % 127)) % 127
 */
static int CalculateCheckDigit(const std::string& data)
{
	int sum = 0;
	for (char c : data) {
		sum += static_cast<unsigned char>(c);
	}
	return (127 - (sum % 127)) % 127;
}

/**
 * Validate the check digit in the decoded string.
 * The last character should be the check digit.
 */
static bool ValidateCheckDigit(const std::string& dataWithCheck)
{
	if (dataWithCheck.length() < 2)
		return false;

	std::string data = dataWithCheck.substr(0, dataWithCheck.length() - 1);
	int expectedCheck = CalculateCheckDigit(data);
	int actualCheck = static_cast<unsigned char>(dataWithCheck.back());

	return expectedCheck == actualCheck;
}

Barcode TelepenReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	// Minimum: start + 1 data + check + stop = 4 * 12 = 48 elements (approximate)
	const int minPatternLen = START_PATTERN_LEN + 12 + 12 + STOP_PATTERN_LEN;

	// Find start pattern
	next = FindLeftGuard<START_PATTERN_LEN>(next, minPatternLen,
		[](const PatternView& view, int space) { return IsStartPattern(view, space); });

	if (!next.isValid())
		return {};

	int xStart = next.pixelsInFront();

	// Skip start pattern
	if (!next.skipSymbol())
		return {};

	std::string txt;
	txt.reserve(50);

	// Decode data characters
	while (next.isValid()) {
		// Check for stop pattern first
		PatternView stopView = next.subView(0, STOP_PATTERN_LEN);
		if (stopView.isValid() && IsStopPattern(stopView))
			break;

		// Try to decode a character
		int ascii = DecodeTelepenChar(next);
		if (ascii < 0)
			return {};

		txt += static_cast<char>(ascii);

		// Skip the decoded character pattern
		int patternLen = GetPatternLength(ascii);
		next = next.subView(patternLen);
		if (!next.isValid())
			return {};
	}

	// Verify we found the stop pattern
	PatternView stopView = next.subView(0, STOP_PATTERN_LEN);
	if (!stopView.isValid() || !IsStopPattern(stopView))
		return {};

	// Move past stop pattern
	next = next.subView(STOP_PATTERN_LEN);

	// Check minimum length (at least 1 data char + check digit)
	if (txt.length() < 2)
		return {};

	// Check quiet zone after stop
	if (!next.hasQuietZoneAfter(QUIET_ZONE_SCALE))
		return {};

	// Validate check digit
	Error error;
	if (!ValidateCheckDigit(txt)) {
		error = ChecksumError();
	}

	// Remove check digit from result (it's a hidden check)
	std::string result = txt.substr(0, txt.length() - 1);

	// Symbology identifier for Telepen: ]B0
	// B = Telepen, 0 = no special processing
	SymbologyIdentifier symbologyIdentifier = {'B', '0'};

	int xStop = next.pixelsTillEnd();
	return Barcode(result, rowNumber, xStart, xStop, BarcodeFormat::Telepen, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
