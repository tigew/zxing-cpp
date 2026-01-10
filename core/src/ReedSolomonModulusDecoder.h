/*
* Copyright 2026 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

namespace ZXing {
namespace Pdf417 {
	class ModulusGF;
}

/**
* Reed-Solomon error correction decoder for prime fields using ModulusGF.
*
* This decoder works with prime Galois fields (e.g., GF(113), GF(929))
* which are different from power-of-2 fields (GF(64), GF(128), GF(256)).
*
* Prime fields use ModulusGF while power-of-2 fields use GenericGF.
* This is why DotCode (GF(113)) and GridMatrix (GF(929)) need a separate
* decoder from the standard ReedSolomonDecode() which only works with GenericGF.
*
* @param field The ModulusGF field to use for error correction
* @param received The received codewords (data + error correction)
* @param numECCodewords Number of error correction codewords
* @param nbErrors Output parameter for number of errors corrected
* @return true if errors were successfully corrected, false otherwise
*/
bool ReedSolomonDecodeModulus(const Pdf417::ModulusGF& field, std::vector<int>& received, int numECCodewords, int& nbErrors);

} // ZXing
