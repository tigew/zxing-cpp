/*
* Copyright 2026 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ZXing {
namespace Pdf417 {
	class ModulusGF;
}

/**
* Shared Modulus Galois Field instances for barcode formats that use prime fields.
*
* Prime fields (GF(p) where p is prime) are different from power-of-2 fields
* (GF(2^n)) and require ModulusGF instead of GenericGF.
*
* Available fields:
* - GF(113) with generator 3: Used by DotCode
* - GF(929) with generator 3: Used by GridMatrix and PDF417
*/

/**
* @return GF(113) field with generator 3, used by DotCode barcode format
*/
const Pdf417::ModulusGF& GetGF113();

/**
* @return GF(929) field with generator 3, used by GridMatrix and PDF417 barcode formats
*/
const Pdf417::ModulusGF& GetGF929();

} // ZXing
