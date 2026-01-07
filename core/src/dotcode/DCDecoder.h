/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ZXing {

class BitMatrix;
class DecoderResult;

namespace DotCode {

/**
 * Decodes a DotCode symbol from a BitMatrix.
 * The BitMatrix should contain the raw symbol dots.
 */
DecoderResult Decode(const BitMatrix& bits);

} // namespace DotCode
} // namespace ZXing
