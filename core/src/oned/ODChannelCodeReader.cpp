/*
* Copyright 2025 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "ODChannelCodeReader.h"

#include "Barcode.h"
#include "ReaderOptions.h"
#include "ZXAlgorithms.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ZXing::OneD {

// Maximum values for each channel (0-indexed by channel-3)
static constexpr std::array<int, 6> MAX_VALUES = {
	26,       // Channel 3: 0-26
	292,      // Channel 4: 0-292
	3493,     // Channel 5: 0-3493
	44072,    // Channel 6: 0-44072
	576688,   // Channel 7: 0-576688
	7742862   // Channel 8: 0-7742862
};

// Number of digits per channel
static constexpr std::array<int, 6> CHANNEL_DIGITS = {2, 3, 4, 5, 6, 7};

// Finder pattern width in modules
static constexpr int FINDER_MODULES = 9;

// Total symbol width in modules per channel (data portion only, excluding finder)
// Channel n: bars sum to specific value based on constraint
static constexpr std::array<int, 6> CHANNEL_WIDTH = {
	11,  // Channel 3
	13,  // Channel 4
	15,  // Channel 5
	17,  // Channel 6
	19,  // Channel 7
	21   // Channel 8
};

/**
 * Decode Channel Code value from bar/space widths.
 *
 * The encoding algorithm enumerates all valid bar/space combinations in a specific order.
 * The numeric value is the ordinal position in this enumeration.
 *
 * For decoding, we iterate through combinations until we find one matching our pattern.
 *
 * @param bars Array of bar widths (1-indexed: bars[0] unused, bars[1..n] are the n bars)
 * @param spaces Array of space widths (1-indexed: spaces[0] unused, spaces[1..n-1] are the spaces)
 * @param channel The channel number (3-8)
 * @return The decoded value, or -1 if invalid
 */
static int64_t DecodeChannelValue(const std::vector<int>& bars, const std::vector<int>& spaces, int channel)
{
	int n = channel;
	int targetWidth = CHANNEL_WIDTH[channel - 3];

	// Verify we have correct number of elements
	if (static_cast<int>(bars.size()) != n + 1 || static_cast<int>(spaces.size()) != n)
		return -1;

	// Calculate total width
	int totalWidth = 0;
	for (int i = 1; i <= n; ++i)
		totalWidth += bars[i];
	for (int i = 1; i < n; ++i)
		totalWidth += spaces[i];

	if (totalWidth != targetWidth)
		return -1;

	// Maximum bar/space widths per channel
	// Derived from Zint implementation constraints
	std::vector<int> maxBar(n + 1);
	std::vector<int> maxSpace(n);

	// Calculate maximum values based on remaining width
	// This is a simplified version - exact maxima depend on position
	for (int i = 1; i <= n; ++i)
		maxBar[i] = 8;
	for (int i = 1; i < n; ++i)
		maxSpace[i] = 8;

	// Count combinations until we find the matching pattern
	// Using the same iteration order as the encoder
	int64_t value = 0;

	std::vector<int> B(n + 1, 1);  // Current bar widths
	std::vector<int> S(n, 1);       // Current space widths

	// Initialize: all elements start at 1
	for (int i = 1; i <= n; ++i)
		B[i] = 1;
	for (int i = 1; i < n; ++i)
		S[i] = 1;

	// Main enumeration loop
	// We iterate through all valid combinations and count until we match
	bool found = false;
	int64_t maxIterations = MAX_VALUES[channel - 3] + 1;

	for (int64_t iter = 0; iter <= maxIterations && !found; ++iter) {
		// Check if current pattern matches
		bool match = true;
		for (int i = 1; i <= n && match; ++i)
			if (B[i] != bars[i])
				match = false;
		for (int i = 1; i < n && match; ++i)
			if (S[i] != spaces[i])
				match = false;

		if (match) {
			value = iter;
			found = true;
			break;
		}

		// Generate next combination
		// The Channel Code enumeration order follows a specific pattern
		// Starting from the rightmost bar/space and incrementing

		// Calculate current width
		int currentWidth = 0;
		for (int i = 1; i <= n; ++i)
			currentWidth += B[i];
		for (int i = 1; i < n; ++i)
			currentWidth += S[i];

		// Increment the last bar if possible
		if (B[n] < maxBar[n] && currentWidth < targetWidth) {
			B[n]++;
		} else {
			// Find the rightmost element we can increment and propagate
			bool incremented = false;

			for (int pos = n - 1; pos >= 1 && !incremented; --pos) {
				// Try incrementing space at position pos
				if (S[pos] < maxSpace[pos]) {
					S[pos]++;
					// Reset all elements to the right
					for (int j = pos + 1; j < n; ++j)
						S[j] = 1;
					for (int j = pos + 1; j <= n; ++j)
						B[j] = 1;
					// Adjust last bar to fill remaining width
					int usedWidth = 0;
					for (int i = 1; i <= n; ++i)
						usedWidth += B[i];
					for (int i = 1; i < n; ++i)
						usedWidth += S[i];
					B[n] = targetWidth - usedWidth + B[n];
					if (B[n] >= 1 && B[n] <= maxBar[n]) {
						incremented = true;
					} else {
						S[pos]--;
					}
				}

				if (!incremented) {
					// Try incrementing bar at position pos
					if (B[pos] < maxBar[pos]) {
						B[pos]++;
						// Reset all elements to the right
						for (int j = pos; j < n; ++j)
							S[j] = 1;
						for (int j = pos + 1; j <= n; ++j)
							B[j] = 1;
						// Adjust last bar to fill remaining width
						int usedWidth = 0;
						for (int i = 1; i <= n; ++i)
							usedWidth += B[i];
						for (int i = 1; i < n; ++i)
							usedWidth += S[i];
						B[n] = targetWidth - usedWidth + B[n];
						if (B[n] >= 1 && B[n] <= maxBar[n]) {
							incremented = true;
						} else {
							B[pos]--;
						}
					}
				}
			}

			if (!incremented) {
				// No more combinations possible
				break;
			}
		}
	}

	if (!found)
		return -1;

	return value;
}

/**
 * Detect finder pattern and extract bar/space widths from pattern.
 *
 * @param next Pattern view to analyze
 * @param channel Output: detected channel number (3-8)
 * @param bars Output: bar widths (1-indexed)
 * @param spaces Output: space widths (1-indexed)
 * @return true if valid Channel Code pattern found
 */
static bool DetectAndExtractPattern(PatternView& next, int& channel,
                                    std::vector<int>& bars, std::vector<int>& spaces)
{
	// Channel Code has a complex structure
	// Minimum elements: finder (special) + channel 3 (3 bars + 2 spaces = 5 elements)
	// We need to detect the finder pattern first

	// The finder is 9 consecutive bar modules
	// After finder comes a space, then alternating bar/space pattern

	int numElements = next.size();
	if (numElements < 6)  // Minimum: finder elements + data elements
		return false;

	// Try to identify the finder pattern
	// In a Channel Code, the first bar(s) form the finder
	// The finder width should be significantly larger than individual data elements

	// Calculate average element width for reference
	int totalWidth = 0;
	for (int i = 0; i < numElements; ++i)
		totalWidth += next[i];
	float avgWidth = static_cast<float>(totalWidth) / numElements;

	// The finder should be about 9 times the module width
	// Data bars are 1-8 modules each

	// For now, use a heuristic: the first bar is the finder if it's much wider
	int finderWidth = next[0];
	float moduleWidth = finderWidth / 9.0f;

	if (moduleWidth < 0.5f)
		return false;

	// Count remaining elements after finder
	int dataElements = numElements - 1;  // -1 for finder bar

	// Determine channel from number of data elements
	// Channel n has n bars + (n-1) spaces = 2n-1 elements after finder
	// But we also have spaces in between, so data region has n bars and n-1 spaces
	// Total data elements: 2n-1 (alternating bar/space/bar/space.../bar)

	// After finder: space + (bar + space) * (n-1) + bar = 1 + 2*(n-1) + 1 = 2n
	// Actually: finder + space + bar + space + bar + ...
	// Pattern is: FINDER | space | bar | space | bar | space | ... | bar

	// Let's count: if we have dataElements elements after the finder bar:
	// dataElements = 2*n - 1 for channel n (alternating bar/space ending in bar)
	// Plus the leading space after finder

	// Actually in PatternView, we get alternating bar/space counts
	// Element 0: first bar (finder)
	// Element 1: space after finder
	// Element 2: first data bar
	// Element 3: first data space
	// ...and so on

	// For channel n:
	// After finder: 1 space + n data bars + (n-1) data spaces = n + (n-1) + 1 = 2n elements
	// Total elements = 1 (finder) + 2n = 2n + 1

	for (int n = 3; n <= 8; ++n) {
		int expectedElements = 2 * n + 1;
		if (numElements == expectedElements) {
			channel = n;

			// Extract bar and space widths
			bars.resize(n + 1);
			spaces.resize(n);

			// Element 0 is finder (skip)
			// Element 1 is space after finder (skip - it's a guard space)
			// Elements 2, 4, 6, ... are data bars
			// Elements 3, 5, 7, ... are data spaces

			for (int i = 1; i <= n; ++i) {
				int elemIdx = 2 * i;  // Data bars at positions 2, 4, 6, ...
				if (elemIdx >= numElements)
					return false;

				// Normalize to module units (round to nearest integer)
				int barModules = static_cast<int>(next[elemIdx] / moduleWidth + 0.5f);
				if (barModules < 1 || barModules > 8)
					return false;
				bars[i] = barModules;
			}

			for (int i = 1; i < n; ++i) {
				int elemIdx = 2 * i + 1;  // Data spaces at positions 3, 5, 7, ...
				if (elemIdx >= numElements)
					return false;

				int spaceModules = static_cast<int>(next[elemIdx] / moduleWidth + 0.5f);
				if (spaceModules < 1 || spaceModules > 8)
					return false;
				spaces[i] = spaceModules;
			}

			// Verify total width matches expected
			int expectedWidth = CHANNEL_WIDTH[n - 3];
			int actualWidth = 0;
			for (int i = 1; i <= n; ++i)
				actualWidth += bars[i];
			for (int i = 1; i < n; ++i)
				actualWidth += spaces[i];

			// Allow some tolerance
			if (std::abs(actualWidth - expectedWidth) <= 1) {
				// Adjust to match expected width if close
				if (actualWidth != expectedWidth) {
					// Adjust the last bar
					bars[n] += (expectedWidth - actualWidth);
					if (bars[n] < 1 || bars[n] > 8)
						return false;
				}
				return true;
			}
		}
	}

	return false;
}

Barcode ChannelCodeReader::decodePattern(int rowNumber, PatternView& next, std::unique_ptr<RowReader::DecodingState>&) const
{
	// Channel Code has varying element counts based on channel
	// Minimum: Channel 3 = 7 elements (finder + 2*3)
	// Maximum: Channel 8 = 17 elements (finder + 2*8)
	constexpr int minElements = 7;
	constexpr float QUIET_ZONE_SCALE = 0.25f;

	// Find a pattern that could be Channel Code
	// The pattern starts with the finder (a wide bar)

	// We need to scan for patterns of varying lengths
	for (int targetElements = 7; targetElements <= 17; targetElements += 2) {
		PatternView view = next;

		if (view.size() < targetElements)
			continue;

		// Look for the finder pattern (first bar should be wide)
		if (view[0] < view[1] * 2)  // Finder should be at least 2x the following space
			continue;

		int channel;
		std::vector<int> bars, spaces;

		if (DetectAndExtractPattern(view, channel, bars, spaces)) {
			// Decode the value
			int64_t value = DecodeChannelValue(bars, spaces, channel);

			if (value >= 0 && value <= MAX_VALUES[channel - 3]) {
				// Format the output with leading zeros
				int numDigits = CHANNEL_DIGITS[channel - 3];
				std::string txt(numDigits, '0');

				int64_t temp = value;
				for (int i = numDigits - 1; i >= 0 && temp > 0; --i) {
					txt[i] = '0' + (temp % 10);
					temp /= 10;
				}

				// Symbology identifier for Channel Code
				SymbologyIdentifier symbologyIdentifier = {'X', '0'};

				int xStart = view.pixelsInFront();
				int xStop = view.pixelsTillEnd();

				return {std::move(txt), rowNumber, xStart, xStop, BarcodeFormat::ChannelCode, symbologyIdentifier};
			}
		}
	}

	return {};
}

} // namespace ZXing::OneD
