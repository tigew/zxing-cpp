/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ZXing {

class BitMatrix;
class DecoderResult;

namespace HanXin {

/**
 * Decodes a Han Xin Code symbol from a BitMatrix.
 * Han Xin Code is a Chinese 2D symbology (GB/T 21049, ISO/IEC 20830).
 */
DecoderResult Decode(const BitMatrix& bits);

} // namespace HanXin
} // namespace ZXing
