/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ZXing {

class BitMatrix;
class DecoderResult;

namespace GridMatrix {

/**
 * Decodes a Grid Matrix symbol from a BitMatrix.
 * Grid Matrix is a 2D matrix symbology optimized for Chinese characters.
 * Specification: GB/T 21049-2007
 */
DecoderResult Decode(const BitMatrix& bits);

} // namespace GridMatrix
} // namespace ZXing
