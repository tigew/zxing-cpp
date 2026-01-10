/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODPOSTNETReader.h"

#include "Barcode.h"
#include "BinaryBitmap.h"
#include "BitMatrix.h"
#include "Content.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "Quadrilateral.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace ZXing::OneD {

// POSTNET digit encoding: 2 tall bars (1) + 3 short bars (0)
// Weights: 7, 4, 2, 1, 0 (left to right)
// Digit 0 is special: uses 7+4 = 11 encoded as 11000
static constexpr uint8_t POSTNET_PATTERNS[10][5] = {
	{1, 1, 0, 0, 0}, // 0: 7+4 = 11 (special)
	{0, 0, 0, 1, 1}, // 1: 0+1 = 1
	{0, 0, 1, 0, 1}, // 2: 0+2 = 2
	{0, 0, 1, 1, 0}, // 3: 1+2 = 3
	{0, 1, 0, 0, 1}, // 4: 0+4 = 4
	{0, 1, 0, 1, 0}, // 5: 1+4 = 5
	{0, 1, 1, 0, 0}, // 6: 2+4 = 6
	{1, 0, 0, 0, 1}, // 7: 0+7 = 7
	{1, 0, 0, 1, 0}, // 8: 1+7 = 8
	{1, 0, 1, 0, 0}, // 9: 2+7 = 9
};

// Valid POSTNET bar counts: 32 (5-digit), 52 (ZIP+4), 62 (DPBC)
static constexpr int POSTNET_LENGTHS[] = {32, 52, 62};

// Valid PLANET bar counts: 62 (12-digit), 72 (14-digit)
static constexpr int PLANET_LENGTHS[] = {62, 72};

/**
 * Decode a 5-bar pattern to a digit for POSTNET (2 tall bars).
 * Returns -1 if no match.
 */
static int DecodePostnetDigit(const uint8_t* pattern)
{
	int tallCount = 0;
	for (int i = 0; i < 5; ++i)
		tallCount += pattern[i];

	if (tallCount != 2)
		return -1;

	for (int digit = 0; digit < 10; ++digit) {
		bool match = true;
		for (int i = 0; i < 5; ++i) {
			if (POSTNET_PATTERNS[digit][i] != pattern[i]) {
				match = false;
				break;
			}
		}
		if (match)
			return digit;
	}
	return -1;
}

/**
 * Decode a 5-bar pattern to a digit for PLANET (3 tall bars, inverse of POSTNET).
 * Returns -1 if no match.
 */
static int DecodePlanetDigit(const uint8_t* pattern)
{
	int tallCount = 0;
	for (int i = 0; i < 5; ++i)
		tallCount += pattern[i];

	if (tallCount != 3)
		return -1;

	// PLANET is inverse of POSTNET: tall becomes short, short becomes tall
	for (int digit = 0; digit < 10; ++digit) {
		bool match = true;
		for (int i = 0; i < 5; ++i) {
			// Inverted: POSTNET 1 -> PLANET 0, POSTNET 0 -> PLANET 1
			uint8_t expected = 1 - POSTNET_PATTERNS[digit][i];
			if (expected != pattern[i]) {
				match = false;
				break;
			}
		}
		if (match)
			return digit;
	}
	return -1;
}

/**
 * Calculate the check digit (mod 10).
 * The check digit makes the sum of all digits a multiple of 10.
 */
static int CalculateCheckDigit(const std::string& data)
{
	int sum = 0;
	for (char c : data) {
		if (c >= '0' && c <= '9')
			sum += c - '0';
	}
	return (10 - (sum % 10)) % 10;
}

/**
 * Validate the check digit in the decoded string.
 */
static bool ValidateCheckDigit(const std::string& dataWithCheck)
{
	if (dataWithCheck.length() < 2)
		return false;

	std::string data = dataWithCheck.substr(0, dataWithCheck.length() - 1);
	int checkDigit = dataWithCheck.back() - '0';

	int expectedCheck = CalculateCheckDigit(data);
	return checkDigit == expectedCheck;
}

// Structure to hold detected barcode region
struct BarcodeRegion {
	int left;
	int right;
	int top;
	int bottom;
	std::vector<int> barCenters;
	std::vector<int> barTops;
	std::vector<int> barBottoms;
	float barWidth;
	float barSpacing;
	bool valid;
};

// Find the vertical extent of a bar at position x
static void FindBarExtent(const BitMatrix& image, int x, int searchTop, int searchBottom,
                          int& barTop, int& barBottom)
{
	barTop = -1;
	barBottom = -1;

	int height = image.height();
	searchTop = std::max(0, searchTop);
	searchBottom = std::min(height - 1, searchBottom);

	// Find top of bar
	for (int y = searchTop; y <= searchBottom; ++y) {
		if (image.get(x, y)) {
			barTop = y;
			break;
		}
	}

	if (barTop < 0)
		return;

	// Find bottom of bar
	for (int y = searchBottom; y >= barTop; --y) {
		if (image.get(x, y)) {
			barBottom = y;
			break;
		}
	}
}

// Detect and analyze barcode region
static BarcodeRegion DetectBarcodeRegion(const BitMatrix& image, bool rotated)
{
	BarcodeRegion region;
	region.valid = false;

	int width = rotated ? image.height() : image.width();
	int height = rotated ? image.width() : image.height();

	if (width < 32 || height < 5)
		return region;

	// Scan middle rows to find bars
	int midY = height / 2;
	int scanStart = midY - height / 4;
	int scanEnd = midY + height / 4;

	// Find left edge of barcode
	int leftEdge = -1;
	for (int x = 0; x < width; ++x) {
		for (int y = scanStart; y <= scanEnd; ++y) {
			int px = rotated ? y : x;
			int py = rotated ? x : y;
			if (image.get(px, py)) {
				leftEdge = x;
				break;
			}
		}
		if (leftEdge >= 0)
			break;
	}

	if (leftEdge < 0)
		return region;

	// Find right edge of barcode
	int rightEdge = -1;
	for (int x = width - 1; x > leftEdge; --x) {
		for (int y = scanStart; y <= scanEnd; ++y) {
			int px = rotated ? y : x;
			int py = rotated ? x : y;
			if (image.get(px, py)) {
				rightEdge = x;
				break;
			}
		}
		if (rightEdge >= 0)
			break;
	}

	if (rightEdge < 0 || rightEdge - leftEdge < 30)
		return region;

	// Detect individual bars by finding bar centers
	std::vector<int> barPositions;
	bool inBar = false;
	int barStart = 0;

	for (int x = leftEdge; x <= rightEdge; ++x) {
		bool hasPixel = false;
		for (int y = scanStart; y <= scanEnd; ++y) {
			int px = rotated ? y : x;
			int py = rotated ? x : y;
			if (image.get(px, py)) {
				hasPixel = true;
				break;
			}
		}

		if (hasPixel && !inBar) {
			barStart = x;
			inBar = true;
		} else if (!hasPixel && inBar) {
			int barCenter = (barStart + x - 1) / 2;
			barPositions.push_back(barCenter);
			inBar = false;
		}
	}

	// Handle bar at end
	if (inBar) {
		int barCenter = (barStart + rightEdge) / 2;
		barPositions.push_back(barCenter);
	}

	int numBars = static_cast<int>(barPositions.size());

	// Check if bar count is valid for POSTNET or PLANET
	bool validLength = false;
	for (int len : POSTNET_LENGTHS) {
		if (numBars == len) {
			validLength = true;
			break;
		}
	}
	if (!validLength) {
		for (int len : PLANET_LENGTHS) {
			if (numBars == len) {
				validLength = true;
				break;
			}
		}
	}

	if (!validLength || numBars < 32)
		return region;

	// Calculate bar spacing
	float totalSpacing = 0;
	for (size_t i = 1; i < barPositions.size(); ++i) {
		totalSpacing += barPositions[i] - barPositions[i - 1];
	}
	float avgSpacing = totalSpacing / (barPositions.size() - 1);

	// Find vertical extent of each bar
	std::vector<int> barTops(numBars);
	std::vector<int> barBottoms(numBars);

	int searchRange = static_cast<int>(height * 0.4f);
	int searchTop = std::max(0, midY - searchRange);
	int searchBottom = std::min(height - 1, midY + searchRange);

	for (int i = 0; i < numBars; ++i) {
		int x = barPositions[i];
		if (rotated) {
			FindBarExtent(image, midY, x - 5, x + 5, barTops[i], barBottoms[i]);
		} else {
			FindBarExtent(image, x, searchTop, searchBottom, barTops[i], barBottoms[i]);
		}
	}

	region.left = leftEdge;
	region.right = rightEdge;
	region.top = searchTop;
	region.bottom = searchBottom;
	region.barCenters = barPositions;
	region.barTops = barTops;
	region.barBottoms = barBottoms;
	region.barSpacing = avgSpacing;
	region.barWidth = avgSpacing * 0.5f;
	region.valid = true;

	return region;
}

// Classify bar heights as tall (1) or short (0)
static std::vector<uint8_t> ClassifyBarHeights(const BarcodeRegion& region)
{
	std::vector<uint8_t> barStates;

	int numBars = static_cast<int>(region.barCenters.size());
	if (numBars < 2)
		return barStates;

	// Calculate bar heights
	std::vector<int> heights(numBars);
	int minHeight = INT_MAX;
	int maxHeight = 0;

	for (int i = 0; i < numBars; ++i) {
		if (region.barTops[i] >= 0 && region.barBottoms[i] >= 0) {
			heights[i] = region.barBottoms[i] - region.barTops[i] + 1;
			minHeight = std::min(minHeight, heights[i]);
			maxHeight = std::max(maxHeight, heights[i]);
		} else {
			heights[i] = 0;
		}
	}

	if (maxHeight <= minHeight)
		return barStates;

	// Threshold for tall vs short bars
	int threshold = (minHeight + maxHeight) / 2;

	barStates.reserve(numBars);
	for (int i = 0; i < numBars; ++i) {
		barStates.push_back(heights[i] > threshold ? 1 : 0);
	}

	return barStates;
}

// Decode bar states to text for POSTNET
static std::string DecodePostnet(const std::vector<uint8_t>& barStates)
{
	int numBars = static_cast<int>(barStates.size());

	// Verify frame bars (first and last should be tall)
	if (barStates.front() != 1 || barStates.back() != 1)
		return "";

	// Number of data bars (excluding frame bars)
	int dataBars = numBars - 2;

	// Must be divisible by 5 for digit encoding
	if (dataBars % 5 != 0)
		return "";

	int numDigits = dataBars / 5;

	std::string result;
	result.reserve(numDigits);

	for (int d = 0; d < numDigits; ++d) {
		int offset = 1 + d * 5; // Skip start frame bar
		uint8_t pattern[5];
		for (int i = 0; i < 5; ++i) {
			pattern[i] = barStates[offset + i];
		}

		int digit = DecodePostnetDigit(pattern);
		if (digit < 0)
			return "";

		result += static_cast<char>('0' + digit);
	}

	return result;
}

// Decode bar states to text for PLANET
static std::string DecodePlanet(const std::vector<uint8_t>& barStates)
{
	int numBars = static_cast<int>(barStates.size());

	// Verify frame bars (first and last should be tall)
	if (barStates.front() != 1 || barStates.back() != 1)
		return "";

	// Number of data bars (excluding frame bars)
	int dataBars = numBars - 2;

	// Must be divisible by 5 for digit encoding
	if (dataBars % 5 != 0)
		return "";

	int numDigits = dataBars / 5;

	std::string result;
	result.reserve(numDigits);

	for (int d = 0; d < numDigits; ++d) {
		int offset = 1 + d * 5; // Skip start frame bar
		uint8_t pattern[5];
		for (int i = 0; i < 5; ++i) {
			pattern[i] = barStates[offset + i];
		}

		int digit = DecodePlanetDigit(pattern);
		if (digit < 0)
			return "";

		result += static_cast<char>('0' + digit);
	}

	return result;
}

Barcode POSTNETReader::decodeInternal(const BitMatrix& image, bool tryRotated) const
{
	BarcodeRegion region = DetectBarcodeRegion(image, tryRotated);
	if (!region.valid)
		return {};

	std::vector<uint8_t> barStates = ClassifyBarHeights(region);
	if (barStates.empty())
		return {};

	int numBars = static_cast<int>(barStates.size());

	// Determine format based on bar count
	BarcodeFormat format = BarcodeFormat::None;
	std::string text;

	// Try POSTNET first (32, 52, or 62 bars)
	bool isPostnetLength = false;
	for (int len : POSTNET_LENGTHS) {
		if (numBars == len) {
			isPostnetLength = true;
			break;
		}
	}

	if (isPostnetLength && _opts.formats().testFlag(BarcodeFormat::POSTNET)) {
		text = DecodePostnet(barStates);
		if (!text.empty() && ValidateCheckDigit(text)) {
			format = BarcodeFormat::POSTNET;
		}
	}

	// Try PLANET if POSTNET didn't match (62 or 72 bars)
	if (format == BarcodeFormat::None) {
		bool isPlanetLength = false;
		for (int len : PLANET_LENGTHS) {
			if (numBars == len) {
				isPlanetLength = true;
				break;
			}
		}

		if (isPlanetLength && _opts.formats().testFlag(BarcodeFormat::PLANET)) {
			text = DecodePlanet(barStates);
			if (!text.empty() && ValidateCheckDigit(text)) {
				format = BarcodeFormat::PLANET;
			}
		}
	}

	// Try reversed if forward didn't work
	if (format == BarcodeFormat::None) {
		std::reverse(barStates.begin(), barStates.end());

		if (isPostnetLength && _opts.formats().testFlag(BarcodeFormat::POSTNET)) {
			text = DecodePostnet(barStates);
			if (!text.empty() && ValidateCheckDigit(text)) {
				format = BarcodeFormat::POSTNET;
			}
		}

		if (format == BarcodeFormat::None) {
			bool isPlanetLength = false;
			for (int len : PLANET_LENGTHS) {
				if (numBars == len) {
					isPlanetLength = true;
					break;
				}
			}

			if (isPlanetLength && _opts.formats().testFlag(BarcodeFormat::PLANET)) {
				text = DecodePlanet(barStates);
				if (!text.empty() && ValidateCheckDigit(text)) {
					format = BarcodeFormat::PLANET;
				}
			}
		}
	}

	if (format == BarcodeFormat::None)
		return {};

	// Build result
	Error error;
	if (!ValidateCheckDigit(text))
		error = ChecksumError();

	// Symbology identifier
	SymbologyIdentifier symbologyIdentifier = {'X', '0'};

	// Create position from barcode region
	QuadrilateralI position;
	if (tryRotated) {
		position = {
			PointI(region.top, region.left),
			PointI(region.bottom, region.left),
			PointI(region.bottom, region.right),
			PointI(region.top, region.right)
		};
	} else {
		position = {
			PointI(region.left, region.top),
			PointI(region.right, region.top),
			PointI(region.right, region.bottom),
			PointI(region.left, region.bottom)
		};
	}

	// Create Content from the decoded string
	ByteArray bytes(text);
	Content contentObj(std::move(bytes), symbologyIdentifier);

	// Create DecoderResult and DetectorResult
	DecoderResult decoderResult(std::move(contentObj));
	decoderResult.setError(error);
	DetectorResult detectorResult({}, std::move(position));

	return Barcode(std::move(decoderResult), std::move(detectorResult), format);
}

Barcode POSTNETReader::decode(const BinaryBitmap& image) const
{
	auto binImg = image.getBitMatrix();
	if (!binImg)
		return {};

	Barcode result = decodeInternal(*binImg, false);

	if (!result.isValid() && _opts.tryRotate()) {
		result = decodeInternal(*binImg, true);
	}

	return result;
}

Barcodes POSTNETReader::decode(const BinaryBitmap& image, [[maybe_unused]] int maxSymbols) const
{
	Barcodes results;

	Barcode result = decode(image);
	if (result.isValid())
		results.push_back(std::move(result));

	return results;
}

} // namespace ZXing::OneD
