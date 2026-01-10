/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODIATA2of5Reader.h"

#include "Barcode.h"
#include "Pattern.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <string>

namespace ZXing::OneD {

// IATA 2 of 5 digit patterns (same as Industrial 2 of 5)
// Each digit is encoded with 10 elements: bar-space-bar-space-bar-space-bar-space-bar-space
// Only bars encode data (2 wide, 3 narrow), spaces are all narrow (fixed width)
// Values: 1 = narrow, 3 = wide
static constexpr std::array<std::array<int, 10>, 10> DIGIT_PATTERNS = {{
	{1, 1, 1, 1, 3, 1, 1, 3, 1, 1}, // 0
	{3, 1, 1, 1, 1, 1, 1, 1, 3, 1}, // 1
	{1, 1, 3, 1, 1, 1, 1, 1, 3, 1}, // 2
	{3, 1, 3, 1, 1, 1, 1, 1, 1, 1}, // 3
	{1, 1, 1, 1, 3, 1, 1, 1, 3, 1}, // 4
	{3, 1, 1, 1, 3, 1, 1, 1, 1, 1}, // 5
	{1, 1, 3, 1, 3, 1, 1, 1, 1, 1}, // 6
	{1, 1, 1, 1, 1, 1, 1, 3, 3, 1}, // 7
	{3, 1, 1, 1, 1, 1, 1, 3, 1, 1}, // 8
	{1, 1, 3, 1, 1, 1, 1, 3, 1, 1}, // 9
}};

// IATA Start pattern: 1,1,1,1 (4 elements: narrow-bar, narrow-space, narrow-bar, narrow-space)
static constexpr FixedPattern<4, 4> START_PATTERN = {1, 1, 1, 1};

// IATA Stop pattern: 3,1,1 (3 elements: wide-bar, narrow-space, narrow-bar)
static constexpr FixedPattern<3, 5> STOP_PATTERN = {3, 1, 1};

// Decode a 10-element pattern into a digit using threshold
// Only bars (even indices) are examined for wide/narrow; spaces (odd indices) should be narrow
static int DecodeDigitWithThreshold(const PatternView& view, const BarAndSpaceI& threshold)
{
	if (!threshold.isValid())
		return -1;

	// Extract bar widths (indices 0, 2, 4, 6, 8)
	std::array<int, 5> barPattern;
	int wideCount = 0;
	for (int i = 0; i < 5; ++i) {
		bool isWide = view[i * 2] > threshold.bar;
		barPattern[i] = isWide ? 3 : 1;
		if (isWide)
			wideCount++;
	}

	// IATA 2 of 5: exactly 2 of 5 bars must be wide
	if (wideCount != 2)
		return -1;

	// Match against known patterns (only checking bar positions)
	for (int digit = 0; digit < 10; ++digit) {
		bool match = true;
		for (int i = 0; i < 5; ++i) {
			if (barPattern[i] != DIGIT_PATTERNS[digit][i * 2]) {
				match = false;
				break;
			}
		}
		if (match)
			return digit;
	}

	return -1;
}

// Validate modulo 10 check digit (optional, same as UPC/GTIN)
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

Barcode IATA2of5Reader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	const int minCharCount = 1;
	const int minQuietZone = 10;

	// Find start pattern
	// IATA Start pattern: 4 elements (narrow-bar, narrow-space, narrow-bar, narrow-space)
	// Need enough space for start + at least 1 digit + stop
	next = FindLeftGuard(next, 4 + 10 + 3, START_PATTERN, minQuietZone);
	if (!next.isValid())
		return {};

	int xStart = next.pixelsInFront();

	// Move past start pattern to first digit
	next = next.subView(4, 10);
	if (!next.isValid())
		return {};

	std::string txt;
	txt.reserve(20);

	// Decode digits
	while (next.isValid()) {
		// Calculate current narrow/wide threshold
		auto threshold = NarrowWideThreshold(next);
		if (!threshold.isValid())
			break;

		// Check if we have enough elements for a digit
		if (next.size() < 10) {
			break;
		}

		// Try to decode the digit
		int digit = DecodeDigitWithThreshold(next, threshold);
		if (digit < 0) {
			// Could be stop pattern or invalid
			break;
		}

		txt.push_back('0' + digit);

		// Move to next character (10 elements per digit)
		next = next.subView(10, 10);
		if (!next.isValid()) {
			// We might be at the stop pattern
			next = next.subView(0, 3);
			break;
		}
	}

	// Verify stop pattern
	// We need at least 3 elements for the stop pattern
	if (!next.isValid() || next.size() < 3)
		return {};

	auto stopView = next.subView(0, 3);
	if (!stopView.isValid())
		return {};

	// Verify stop pattern: wide-bar, narrow-space, narrow-bar
	// Check that first bar is wide and last bar is narrow
	auto threshold = NarrowWideThreshold(stopView);
	if (threshold.isValid()) {
		// First bar (index 0) should be wide
		if (stopView[0] < threshold.bar)
			return {};
		// Last bar (index 2) should be narrow
		if (stopView[2] > threshold.bar)
			return {};
	} else {
		// Fallback: first bar should be significantly wider than last bar
		if (stopView[0] <= stopView[2])
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
	// Using 'A' for IATA/Airline 2 of 5 (not an official ISO designation)
	// '0' = no check digit verified, '1' = check digit verified
	SymbologyIdentifier symbologyIdentifier = {'A', checksumValid ? '1' : '0'};

	return Barcode(txt, rowNumber, xStart, xStop, BarcodeFormat::IATA2of5, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
