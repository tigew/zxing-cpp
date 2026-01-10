/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <cstdint>

namespace ZXing::CodeOne {

/**
 * Code One version information.
 * Based on AIM USS-Code One (ISO/IEC 11543).
 */
struct Version {
	char name;           // 'A' through 'H', 'S', or 'T'
	int width;           // Symbol width in modules
	int height;          // Symbol height in modules
	int dataCodewords;   // Number of data codewords
	int ecCodewords;     // Number of error correction codewords
	int dataRegionWidth; // Width of a data region
	int dataRegionHeight;// Height of a data region
	int hRegions;        // Number of horizontal data regions
	int vRegions;        // Number of vertical data regions
};

// Code One versions A-H (main versions)
// Version S and T are simpler linear variants
constexpr Version VERSIONS[] = {
	{'A', 16, 18,  13,  10, 16, 18, 1, 1},  // Version A: 16x18
	{'B', 22, 22,  22,  16, 22, 22, 1, 1},  // Version B: 22x22
	{'C', 28, 32,  44,  26, 28, 32, 1, 1},  // Version C: 28x32
	{'D', 40, 42,  91,  44, 20, 21, 2, 2},  // Version D: 40x42 (2x2 regions)
	{'E', 52, 54, 155,  70, 26, 27, 2, 2},  // Version E: 52x54 (2x2 regions)
	{'F', 70, 76, 271, 140, 35, 38, 2, 2},  // Version F: 70x76 (2x2 regions)
	{'G',104, 98, 480, 280, 26, 49, 4, 2},  // Version G: 104x98 (4x2 regions)
	{'H',148,134, 975, 560, 37, 67, 4, 2},  // Version H: 148x134 (4x2 regions)
};

constexpr int NUM_VERSIONS = sizeof(VERSIONS) / sizeof(VERSIONS[0]);

/**
 * Find version by dimensions.
 * Returns nullptr if no matching version found.
 */
inline const Version* FindVersion(int width, int height) {
	for (const auto& v : VERSIONS) {
		if (v.width == width && v.height == height)
			return &v;
	}
	return nullptr;
}

/**
 * Find version by name.
 * Returns nullptr if no matching version found.
 */
inline const Version* FindVersion(char name) {
	for (const auto& v : VERSIONS) {
		if (v.name == name)
			return &v;
	}
	return nullptr;
}

} // namespace ZXing::CodeOne
