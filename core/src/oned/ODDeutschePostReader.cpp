/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODDeutschePostReader.h"

#include "ReaderOptions.h"
#include "Barcode.h"
#include "ZXAlgorithms.h"

namespace ZXing::OneD {

/**
 * Validates the Deutsche Post check digit using weights 4 and 9.
 *
 * Algorithm:
 * 1. Multiply each digit by alternating weights (4, 9, 4, 9, ...)
 * 2. Sum all products
 * 3. Check digit = (10 - (sum mod 10)) mod 10
 *
 * @param txt The complete barcode text including check digit
 * @return true if check digit is valid
 */
static bool IsCheckDigitValid(const std::string& txt)
{
	if (txt.length() < 2)
		return false;

	int sum = 0;
	// Weights alternate: 4, 9, 4, 9, ... for positions 0, 1, 2, 3, ...
	for (size_t i = 0; i < txt.length() - 1; ++i) {
		int digit = txt[i] - '0';
		int weight = (i % 2 == 0) ? 4 : 9;
		sum += digit * weight;
	}

	int expectedCheck = (10 - (sum % 10)) % 10;
	int actualCheck = txt.back() - '0';

	return expectedCheck == actualCheck;
}

Barcode DeutschePostReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const
{
	const int minQuietZone = 6; // spec requires 10

	// ITF start pattern: 4 narrow elements (1,1,1,1)
	next = FindLeftGuard(next, 4 + 10 + 3, FixedPattern<4, 4>{1, 1, 1, 1}, minQuietZone);
	if (!next.isValid())
		return {};

	// Get threshold of first character pair
	auto threshold = NarrowWideThreshold(next.subView(4, 10));
	if (!threshold.isValid())
		return {};

	// Check that each bar/space in the start pattern is < threshold (narrow)
	for (int i = 0; i < 4; ++i)
		if (next[i] > threshold[i])
			return {};

	constexpr int weights[] = {1, 2, 4, 7, 0};
	int xStart = next.pixelsInFront();

	next = next.subView(4, 10);

	std::string txt;
	txt.reserve(16);

	while (next.isValid()) {
		// Look for end-of-symbol (wide space indicates stop pattern)
		if (next[3] > threshold.space * 3)
			break;

		BarAndSpace<int> digits, numWide;
		bool bad = false;
		for (int i = 0; i < 10; ++i) {
			bad |= next[i] > threshold[i] * 3 || next[i] < threshold[i] / 3;
			numWide[i] += next[i] > threshold[i];
			digits[i] += weights[i/2] * (next[i] > threshold[i]);
		}

		// ITF requires exactly 2 wide bars and 2 wide spaces per character pair
		if (bad || numWide.bar != 2 || numWide.space != 2)
			break;

		for (int i = 0; i < 2; ++i)
			txt.push_back(ToDigit(digits[i] == 11 ? 0 : digits[i]));

		// Update threshold for scanning slanted symbols
		threshold = NarrowWideThreshold(next);

		next.skipSymbol();
	}

	next = next.subView(0, 3);

	// Check stop pattern: wide-narrow-narrow (2,1,1)
	if (!next.isValid() || !threshold.isValid()
		|| next[0] < threshold[0] || next[1] > threshold[1] || next[2] > threshold[2])
		return {};

	// Check quiet zone size
	if (!(std::min((int)next[3], xStart) > minQuietZone * (threshold.bar + threshold.space) / 3))
		return {};

	// Determine format based on length
	BarcodeFormat format = BarcodeFormat::None;
	size_t len = txt.length();

	if (len == 14) {
		// Leitcode: 14 digits
		if (_opts.formats().testFlag(BarcodeFormat::DeutschePostLeitcode))
			format = BarcodeFormat::DeutschePostLeitcode;
	} else if (len == 12) {
		// Identcode: 12 digits
		if (_opts.formats().testFlag(BarcodeFormat::DeutschePostIdentcode))
			format = BarcodeFormat::DeutschePostIdentcode;
	}

	if (format == BarcodeFormat::None)
		return {};

	// Validate check digit
	Error error;
	if (!IsCheckDigitValid(txt))
		error = ChecksumError();

	// Symbology identifier: 'I' for Interleaved 2 of 5
	// Modifier '1' if checksum is valid, '0' otherwise
	SymbologyIdentifier symbologyIdentifier = {'I', IsCheckDigitValid(txt) ? '1' : '0'};

	int xStop = next.pixelsTillEnd();
	return Barcode(txt, rowNumber, xStart, xStop, format, symbologyIdentifier, error);
}

} // namespace ZXing::OneD
