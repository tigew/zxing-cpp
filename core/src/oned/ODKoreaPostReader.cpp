/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODKoreaPostReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <cmath>
#include <string>

namespace ZXing::OneD {

// Korea Post encoding table
// Each string represents alternating bar/space widths in module units
// A '0' at a bar position means no bar at that location
// Patterns represent: (bar, space) pairs, variable length
static const char* KOREA_TABLE[10] = {
	"1313150613",  // 0
	"0713131313",  // 1 - starts with space (no leading bar)
	"0417131313",  // 2 - starts with space
	"1506131313",  // 3
	"0413171313",  // 4 - starts with space
	"17171313",    // 5
	"1315061313",  // 6
	"0413131713",  // 7 - starts with space
	"17131713",    // 8
	"13171713"     // 9
};

// Module counts per digit (sum of pattern values, excluding zeros)
static constexpr int MODULES_PER_DIGIT = 24;  // Most digits use 24 modules

// Total digits: 6 data digits + 1 check digit = 7
static constexpr int TOTAL_DIGITS = 7;

// Minimum quiet zone as fraction of digit width
static constexpr float QUIET_ZONE_FACTOR = 0.5f;

// Maximum variance allowed when matching patterns
static constexpr float MAX_AVG_VARIANCE = 0.38f;
static constexpr float MAX_INDIVIDUAL_VARIANCE = 0.7f;

// Parse pattern string into bar/space widths array
// Returns the number of elements (bars + spaces)
static int ParsePattern(const char* patternStr, int* bars, int* spaces, int maxPairs)
{
	int pairs = 0;
	int len = static_cast<int>(strlen(patternStr));

	for (int i = 0; i + 1 < len && pairs < maxPairs; i += 2) {
		bars[pairs] = patternStr[i] - '0';
		spaces[pairs] = patternStr[i + 1] - '0';
		pairs++;
	}

	return pairs;
}

// Try to decode a single digit from the pattern view
// Returns the decoded digit (0-9) or -1 if no match
static int DecodeKoreaPostDigit(const PatternView& view, float /*moduleWidth*/)
{
	// Korea Post digits have variable structure:
	// - Most have 4 bars and 4-5 spaces
	// - The pattern encodes bar/space widths in modules

	// Collect the bar/space widths from the view
	if (view.size() < 4)  // Need at least 4 elements (2 bars, 2 spaces)
		return -1;

	float bestVariance = MAX_AVG_VARIANCE;
	int bestMatch = -1;

	for (int digit = 0; digit < 10; digit++) {
		const char* pattern = KOREA_TABLE[digit];

		// Parse expected bar/space widths
		int expectedBars[5], expectedSpaces[5];
		int pairs = ParsePattern(pattern, expectedBars, expectedSpaces, 5);

		// Calculate total expected modules
		int totalExpected = 0;
		for (int i = 0; i < pairs; i++) {
			totalExpected += expectedBars[i] + expectedSpaces[i];
		}

		// Observed total
		int totalObserved = view.sum();

		// Check if view length matches expected elements
		// A pattern with N pairs has (up to) N bars + N spaces = 2N elements
		// But zero-width bars reduce the actual count
		int expectedElements = 2 * pairs;
		for (int i = 0; i < pairs; i++) {
			if (expectedBars[i] == 0)
				expectedElements--;
		}

		// Skip if element count doesn't match
		if (view.size() != expectedElements && view.size() != expectedElements - 1)
			continue;

		// Calculate variance between observed and expected patterns
		float unitWidth = static_cast<float>(totalObserved) / totalExpected;
		float totalVariance = 0.0f;
		float maxIndVar = MAX_INDIVIDUAL_VARIANCE * unitWidth;
		bool valid = true;

		int viewIdx = 0;

		// If pattern starts with space (bar width 0), view should start with space too
		// But actually, patterns starting with 0 bar mean the digit starts with a space
		// In the barcode context, this would merge with the previous digit's trailing space

		for (int i = 0; i < pairs && viewIdx < view.size() && valid; i++) {
			// Bar
			if (expectedBars[i] != 0) {
				if (viewIdx >= view.size()) {
					valid = false;
					break;
				}
				float variance = std::abs(view[viewIdx] - expectedBars[i] * unitWidth);
				if (variance > maxIndVar) {
					valid = false;
					break;
				}
				totalVariance += variance;
				viewIdx++;
			}

			// Space (skip last space as it merges with next digit)
			if (i < pairs - 1 || true) {  // Include trailing space for now
				if (viewIdx >= view.size()) {
					// Missing trailing space is OK
					break;
				}
				float variance = std::abs(view[viewIdx] - expectedSpaces[i] * unitWidth);
				if (variance > maxIndVar) {
					valid = false;
					break;
				}
				totalVariance += variance;
				viewIdx++;
			}
		}

		if (!valid)
			continue;

		float avgVariance = totalVariance / totalObserved;
		if (avgVariance < bestVariance) {
			bestVariance = avgVariance;
			bestMatch = digit;
		}
	}

	return bestMatch;
}

// Calculate Korea Post check digit (modulo 10)
static int CalculateCheckDigit(const std::string& digits)
{
	int sum = 0;
	for (char c : digits) {
		sum += c - '0';
	}
	int check = 10 - (sum % 10);
	return check == 10 ? 0 : check;
}

Barcode KoreaPostReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	// Korea Post structure:
	// - 6 data digits + 1 check digit = 7 digits
	// - Each digit is approximately 24 modules
	// - Total width: approximately 7 * 24 = 168 modules

	// Look for a pattern that could be Korea Post
	// Each digit has 4-5 bar/space pairs

	// Minimum elements: 7 digits * ~8 elements = ~56 bar/space elements
	// Maximum: 7 digits * 10 elements = 70 elements
	constexpr int MIN_PATTERN_SIZE = 40;  // Conservative minimum

	// Try to find a valid Korea Post barcode
	while (next.isValid() && next.size() >= MIN_PATTERN_SIZE) {
		// Check for quiet zone before pattern
		if (next.pixelsInFront() < 5) {
			next.shift(1);
			continue;
		}

		// Try to decode 7 digits starting from current position
		std::string result;
		result.reserve(TOTAL_DIGITS);

		PatternView current = next;
		int xStart = current.pixelsInFront();
		bool valid = true;

		// Estimate module width from first few elements
		// Korea Post patterns have varied widths, typical module is narrow bar width
		int initialSum = 0;
		int initialCount = std::min(10, current.size());
		for (int i = 0; i < initialCount; i++) {
			initialSum += current[i];
		}
		float moduleWidth = static_cast<float>(initialSum) / (initialCount * 2);  // Rough estimate

		for (int d = 0; d < TOTAL_DIGITS && valid; d++) {
			// Try different window sizes for each digit
			int bestDigit = -1;
			int bestWindowSize = 0;

			// Digits have 8-10 elements typically
			for (int windowSize = 7; windowSize <= 10 && bestDigit == -1 && current.size() >= windowSize; windowSize++) {
				PatternView digitView = current.subView(0, windowSize);
				int digit = DecodeKoreaPostDigit(digitView, moduleWidth);
				if (digit >= 0) {
					bestDigit = digit;
					bestWindowSize = windowSize;
				}
			}

			if (bestDigit == -1) {
				valid = false;
				break;
			}

			result += ('0' + bestDigit);
			current.shift(bestWindowSize);
		}

		if (!valid || result.length() != TOTAL_DIGITS) {
			next.shift(1);
			continue;
		}

		// Verify check digit
		std::string dataDigits = result.substr(0, 6);
		int expectedCheck = CalculateCheckDigit(dataDigits);
		int actualCheck = result[6] - '0';

		if (expectedCheck != actualCheck) {
			next.shift(1);
			continue;
		}

		// Check for quiet zone after pattern
		int xStop = current.pixelsTillEnd();

		// Korea Post symbology identifier
		SymbologyIdentifier symbologyIdentifier = {'X', '0'};

		// Return the 6 data digits (exclude check digit from result)
		return Barcode(dataDigits, rowNumber, xStart, xStop, BarcodeFormat::KoreaPost, symbologyIdentifier);
	}

	return {};
}

} // namespace ZXing::OneD
