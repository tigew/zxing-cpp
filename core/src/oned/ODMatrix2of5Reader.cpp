/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODMatrix2of5Reader.h"

#include "Barcode.h"
#include "Pattern.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <string>

namespace ZXing::OneD {

// Matrix 2 of 5 digit patterns
// Each digit is encoded with 6 elements: bar-space-bar-space-bar-space
// Values: 1 = narrow, 3 = wide
// Two of five elements (excluding the last space) are wide
static constexpr std::array<std::array<int, 6>, 10> DIGIT_PATTERNS = {{
	{1, 1, 3, 3, 1, 1}, // 0
	{3, 1, 1, 1, 3, 1}, // 1
	{1, 3, 1, 1, 3, 1}, // 2
	{3, 3, 1, 1, 1, 1}, // 3
	{1, 1, 3, 1, 3, 1}, // 4
	{3, 1, 3, 1, 1, 1}, // 5
	{1, 3, 3, 1, 1, 1}, // 6
	{1, 1, 1, 3, 3, 1}, // 7
	{3, 1, 1, 3, 1, 1}, // 8
	{1, 3, 1, 3, 1, 1}, // 9
}};

// Start pattern: wide bar followed by narrow space-bar-space-bar-space (6 elements)
// Pattern: 4,1,1,1,1,1 - extra wide start bar distinguishes from data
static constexpr FixedPattern<6, 10> START_PATTERN = {4, 1, 1, 1, 1, 1};

// Stop pattern: wide bar followed by narrow space-bar-space-bar (5 elements)
// Pattern: 4,1,1,1,1
static constexpr FixedPattern<5, 8> STOP_PATTERN = {4, 1, 1, 1, 1};

// Decode a 6-element pattern into a digit using threshold
static int DecodeDigitWithThreshold(const PatternView& view, const BarAndSpaceI& threshold)
{
	if (!threshold.isValid())
		return -1;

	// Convert pattern to wide/narrow sequence
	std::array<int, 6> pattern;
	int wideCount = 0;
	for (int i = 0; i < 6; ++i) {
		bool isWide = view[i] > threshold[i];
		pattern[i] = isWide ? 3 : 1;
		// Count wide elements in bars and spaces (positions 0-4)
		// Position 5 (last space) doesn't count for the "2 of 5" rule
		if (i < 5 && isWide)
			wideCount++;
	}

	// Matrix 2 of 5: exactly 2 of the first 5 elements must be wide
	if (wideCount != 2)
		return -1;

	// Match against known patterns
	for (int digit = 0; digit < 10; ++digit) {
		bool match = true;
		for (int i = 0; i < 6; ++i) {
			if (pattern[i] != DIGIT_PATTERNS[digit][i]) {
				match = false;
				break;
			}
		}
		if (match)
			return digit;
	}

	return -1;
}

// Validate mod 10/3 check digit (optional)
static bool ValidateCheckDigit(const std::string& data)
{
	if (data.length() < 2)
		return false;

	int sum = 0;
	// Process from right to left, excluding check digit
	for (size_t i = 0; i < data.length() - 1; ++i) {
		int digit = data[data.length() - 2 - i] - '0';
		// Odd positions (1st, 3rd, 5th from right) are multiplied by 3
		// Even positions are multiplied by 1
		sum += digit * ((i % 2 == 0) ? 3 : 1);
	}

	int checkDigit = (10 - (sum % 10)) % 10;
	return (data.back() - '0') == checkDigit;
}

Barcode Matrix2of5Reader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	const int minCharCount = 1;
	const int minQuietZone = 10;

	// Find start pattern
	// Start pattern: 6 elements (wide bar + 5 narrow elements)
	// Need enough space for start + at least 1 digit + stop
	next = FindLeftGuard(next, 6 + 6 + 5, START_PATTERN, minQuietZone);
	if (!next.isValid())
		return {};

	int xStart = next.pixelsInFront();

	// Move past start pattern
	next = next.subView(6, 6);
	if (!next.isValid())
		return {};

	std::string txt;
	txt.reserve(20);

	// Decode digits
	while (next.isValid()) {
		// Check if we've reached the stop pattern
		// Stop pattern starts with an extra-wide bar (much wider than normal wide)
		// The first bar of a digit character is either narrow or wide (1 or 3 units)
		// The stop bar is 4 units - significantly wider

		// Calculate current narrow/wide threshold
		auto threshold = NarrowWideThreshold(next);
		if (!threshold.isValid())
			break;

		// If the first bar is more than 2.5x the threshold, it's likely the stop pattern
		if (next[0] > threshold.bar * 2.5) {
			break;
		}

		// Decode the digit
		int digit = DecodeDigitWithThreshold(next, threshold);
		if (digit < 0)
			break;

		txt.push_back('0' + digit);

		// Move to next character
		// After 6 elements, there's an inter-character gap (narrow space)
		// The next view starts at the next bar
		next = next.subView(6, 6);
		if (!next.isValid()) {
			// We might be at the stop pattern, need to check
			next = next.subView(0, 5);
			break;
		}
	}

	// Verify stop pattern
	// At this point, next should be positioned at or before the stop pattern
	// We need at least 5 elements for the stop pattern
	if (!next.isValid() || next.size() < 5)
		return {};

	// Verify stop pattern
	// The stop pattern is: wide-bar, narrow-space, narrow-bar, narrow-space, narrow-bar
	auto stopView = next.subView(0, 5);
	if (!stopView.isValid())
		return {};

	// Check that first bar is extra-wide (stop indicator)
	auto threshold = NarrowWideThreshold(stopView.subView(1, 4));
	if (!threshold.isValid()) {
		// If threshold calculation fails, use a simpler check
		int total = 0;
		for (int i = 0; i < 5; ++i)
			total += stopView[i];
		int avg = total / 5;
		// First bar should be significantly wider than average
		if (stopView[0] < avg * 1.5)
			return {};
	} else {
		// First bar should be extra wide (like start pattern)
		if (stopView[0] < threshold.bar * 2)
			return {};
	}

	// Check minimum character count
	if (Size(txt) < minCharCount)
		return {};

	next = stopView;
	int xStop = next.pixelsTillEnd();

	// Check for valid checksum if enabled
	Error error;
	bool checksumValid = false;
	if (txt.length() >= 2) {
		checksumValid = ValidateCheckDigit(txt);
	}

	// Symbology identifier: based on check digit presence
	// Using 'S' for Standard/Matrix 2 of 5 (not an official ISO designation)
	// '0' = no check digit verified, '1' = check digit verified
	SymbologyIdentifier symbologyIdentifier = {'S', checksumValid ? '1' : '0'};

	return Barcode(txt, rowNumber, xStart, xStop, BarcodeFormat::Matrix2of5, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
