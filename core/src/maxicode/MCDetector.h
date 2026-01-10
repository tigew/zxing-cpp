/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2026 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ZXing {

class BitMatrix;
class DetectorResult;

namespace MaxiCode {

/**
 * @brief Detects a MaxiCode symbol in an image
 *
 * @param image The BitMatrix to search
 * @param isPure Whether the image contains only a pure barcode (no rotation/skew)
 * @param tryHarder Whether to try harder to find the pattern (slower but more accurate)
 * @return DetectorResult containing the extracted bits and position, or invalid result if not found
 */
ZXing::DetectorResult Detect(const BitMatrix& image, bool isPure, bool tryHarder = true);

} // MaxiCode
} // ZXing
