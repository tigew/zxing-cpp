/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODPharmacodeReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ZXing::OneD {

// Pharmacode constants
static constexpr int MIN_BARS = 2;
static constexpr int MAX_BARS = 16;
static constexpr int MIN_VALUE = 3;
static constexpr int MAX_VALUE = 131070;

// Wide to narrow ratio is typically 3:1
// We use a threshold of 2:1 to distinguish (midpoint between 1:1 and 3:1)
static constexpr float WIDE_NARROW_THRESHOLD = 2.0f;

// Quiet zone requirement (relative to narrow bar width)
static constexpr float QUIET_ZONE_FACTOR = 3.0f;

/**
 * Classify bars as narrow (false) or wide (true) based on their widths.
 * Returns empty vector if classification fails.
 */
static std::vector<bool> ClassifyBars(const PatternView& view)
{
	// Pharmacode bars are at even indices (0, 2, 4, ...)
	// Spaces are at odd indices (1, 3, 5, ...)

	// Count the number of bars
	int barCount = (view.size() + 1) / 2;
	if (barCount < MIN_BARS || barCount > MAX_BARS)
		return {};

	// Collect bar widths
	std::vector<int> barWidths;
	barWidths.reserve(barCount);
	for (int i = 0; i < view.size(); i += 2) {
		barWidths.push_back(view[i]);
	}

	// Find minimum and maximum bar widths
	int minWidth = *std::min_element(barWidths.begin(), barWidths.end());
	int maxWidth = *std::max_element(barWidths.begin(), barWidths.end());

	// If all bars are the same width, we can't distinguish narrow from wide
	// This is only valid if all are narrow (encodes values like 3, 7, 15, etc.)
	// or all are wide (encodes values like 6, 14, 30, etc.)
	// We'll treat uniform bars as narrow for decoding purposes
	if (maxWidth < minWidth * WIDE_NARROW_THRESHOLD) {
		// All bars are approximately the same width
		// Could be all narrow or all wide - try both interpretations
		// For simplicity, assume all narrow first
		return std::vector<bool>(barCount, false);
	}

	// Calculate threshold as midpoint
	// Narrow bars should be around minWidth, wide bars around 3*minWidth
	// Threshold at 2*minWidth separates them
	float threshold = minWidth * WIDE_NARROW_THRESHOLD;

	// Classify each bar
	std::vector<bool> isWide(barCount);
	for (int i = 0; i < barCount; ++i) {
		isWide[i] = barWidths[i] > threshold;
	}

	return isWide;
}

/**
 * Decode a Pharmacode from the bar classifications.
 * Reading is from right to left.
 *
 * Formula:
 * - Narrow bar at position n (from right, n=0,1,2,...): contributes 2^n
 * - Wide bar at position n: contributes 2 * 2^n = 2^(n+1)
 */
static int DecodeValue(const std::vector<bool>& isWide)
{
	int value = 0;
	int barCount = static_cast<int>(isWide.size());

	// Process bars from right to left (position 0 is rightmost)
	for (int i = 0; i < barCount; ++i) {
		int position = barCount - 1 - i;  // Convert left-to-right index to position from right
		if (isWide[i]) {
			// Wide bar: contributes 2 * 2^position = 2^(position+1)
			value += (2 << position);
		} else {
			// Narrow bar: contributes 2^position
			value += (1 << position);
		}
	}

	return value;
}

/**
 * Try to decode Pharmacode assuming all bars are wide.
 */
static int DecodeValueAllWide(int barCount)
{
	int value = 0;
	for (int i = 0; i < barCount; ++i) {
		value += (2 << i);
	}
	return value;
}

Barcode PharmacodeReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<RowReader::DecodingState>&) const
{
	// Pharmacode has no start/stop patterns, so we look for any sequence of bars
	// Minimum pattern: 2 bars = 3 elements (bar, space, bar)
	constexpr int MIN_PATTERN_SIZE = 3;

	// Skip to first bar (odd index means we're on a space, even means bar)
	while (next.isValid() && next.size() >= MIN_PATTERN_SIZE) {
		// Check for quiet zone before the pattern
		int quietZoneBefore = next.isAtFirstBar() ? next[0] : next[-1];

		// Find a potential pharmacode pattern
		// We'll scan through looking for sequences of bars
		PatternView pattern = next;

		// Collect bars until we hit a space that's too large or run out of pattern
		int barCount = 0;
		int totalBars = 0;
		int minBarWidth = pattern[0];
		int maxBarWidth = pattern[0];

		for (int i = 0; i < pattern.size(); i += 2) {
			int barWidth = pattern[i];
			totalBars++;
			minBarWidth = std::min(minBarWidth, barWidth);
			maxBarWidth = std::max(maxBarWidth, barWidth);

			// Check if next space is too large (would indicate end of code)
			if (i + 1 < pattern.size()) {
				int spaceWidth = pattern[i + 1];
				// Space should be roughly similar to narrow bar width
				// If space is much larger, this might be end of pharmacode
				if (spaceWidth > maxBarWidth * 2) {
					barCount = totalBars;
					break;
				}
			} else {
				barCount = totalBars;
				break;
			}

			if (totalBars >= MAX_BARS) {
				barCount = MAX_BARS;
				break;
			}
		}

		if (barCount < MIN_BARS) {
			next.shift(2);
			next.extend();
			continue;
		}

		// Create a view of just the pharmacode bars and spaces
		int patternLength = barCount * 2 - 1;  // bars and spaces between them
		if (pattern.size() < patternLength) {
			next.shift(2);
			next.extend();
			continue;
		}

		// Create a sub-view for classification
		PatternView codeView = pattern.subView(0, patternLength);

		// Check quiet zone
		float avgNarrowWidth = static_cast<float>(minBarWidth);
		if (quietZoneBefore < avgNarrowWidth * QUIET_ZONE_FACTOR * 0.5f) {
			next.shift(2);
			next.extend();
			continue;
		}

		// Classify bars
		std::vector<bool> isWide = ClassifyBars(codeView);
		if (isWide.empty()) {
			next.shift(2);
			next.extend();
			continue;
		}

		// Decode value
		int value = DecodeValue(isWide);

		// Validate value range
		if (value < MIN_VALUE || value > MAX_VALUE) {
			// Try interpreting all bars as wide if they were all classified as narrow
			if (std::none_of(isWide.begin(), isWide.end(), [](bool b) { return b; })) {
				value = DecodeValueAllWide(barCount);
			}

			if (value < MIN_VALUE || value > MAX_VALUE) {
				next.shift(2);
				next.extend();
				continue;
			}
		}

		// Check quiet zone after
		int quietZoneAfter = patternLength < pattern.size() ? pattern[patternLength] : 0;
		if (quietZoneAfter < avgNarrowWidth * QUIET_ZONE_FACTOR * 0.5f && patternLength < pattern.size()) {
			next.shift(2);
			next.extend();
			continue;
		}

		// Success - build result
		std::string txt = std::to_string(value);

		int xStart = next.pixelsInFront();
		next.shift(patternLength);
		int xStop = next.pixelsTillEnd();

		// Pharmacode has no standard symbology identifier
		// Using 'L' for Laetus (the company that developed it)
		SymbologyIdentifier symbologyIdentifier = {'L', '0'};

		return {std::move(txt), rowNumber, xStart, xStop, BarcodeFormat::Pharmacode, symbologyIdentifier};
	}

	return {};
}

} // namespace ZXing::OneD
