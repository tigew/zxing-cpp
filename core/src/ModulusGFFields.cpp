/*
* Copyright 2026 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#include "ModulusGFFields.h"

#include "pdf417/PDFModulusGF.h"

namespace ZXing {

const Pdf417::ModulusGF& GetGF113()
{
	// GF(113) with generator 3, used by DotCode
	// 113 is prime, so this is a prime field
	static const Pdf417::ModulusGF field(113, 3);
	return field;
}

const Pdf417::ModulusGF& GetGF929()
{
	// GF(929) with generator 3, used by GridMatrix and PDF417
	// 929 is prime, so this is a prime field
	static const Pdf417::ModulusGF field(929, 3);
	return field;
}

} // ZXing
