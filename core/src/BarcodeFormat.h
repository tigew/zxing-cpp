/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Flags.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace ZXing {

/**
* Enumerates barcode formats known to this package.
*/
enum class BarcodeFormat : uint64_t
{
	// The values are an implementation detail. The c++ use-case (ZXing::Flags) could have been designed such that it
	// would not have been necessary to explicitly set the values to single bit constants. This has been done to ease
	// the interoperability with C-like interfaces, the python and the Qt wrapper.
	None            = 0,         ///< Used as a return value if no valid barcode has been detected
	Aztec           = (1 << 0),  ///< Aztec
	Codabar         = (1 << 1),  ///< Codabar
	Code39          = (1 << 2),  ///< Code39
	Code93          = (1 << 3),  ///< Code93
	Code128         = (1 << 4),  ///< Code128
	DataBar         = (1 << 5),  ///< GS1 DataBar, formerly known as RSS 14
	DataBarExpanded = (1 << 6),  ///< GS1 DataBar Expanded, formerly known as RSS EXPANDED
	DataMatrix      = (1 << 7),  ///< DataMatrix
	EAN8            = (1 << 8),  ///< EAN-8
	EAN13           = (1 << 9),  ///< EAN-13
	ITF             = (1 << 10), ///< ITF (Interleaved Two of Five)
	MaxiCode        = (1 << 11), ///< MaxiCode
	PDF417          = (1 << 12), ///< PDF417
	QRCode          = (1 << 13), ///< QR Code
	UPCA            = (1 << 14), ///< UPC-A
	UPCE            = (1 << 15), ///< UPC-E
	MicroQRCode     = (1 << 16), ///< Micro QR Code
	RMQRCode        = (1 << 17), ///< Rectangular Micro QR Code
	DXFilmEdge      = (1 << 18), ///< DX Film Edge Barcode
	DataBarLimited  = (1 << 19), ///< GS1 DataBar Limited
	AustraliaPost   = (1 << 20), ///< Australia Post 4-State (Standard Customer, Reply Paid, Routing, Redirection)
	KIXCode         = (1 << 21), ///< Dutch Post KIX Code 4-State (Netherlands postal)
	JapanPost       = (1 << 22), ///< Japan Post 4-State Customer Code (Kasutama Barcode)
	KoreaPost       = (1 << 23), ///< Korea Post Barcode (Korean Postal Authority Code)
	RM4SCC          = (1 << 24), ///< Royal Mail 4-State Customer Code (UK postal)
	Mailmark        = (1 << 25), ///< Royal Mail 4-State Mailmark (UK postal, Types C and L)
	USPSIMB         = (1 << 26), ///< USPS Intelligent Mail Barcode (OneCode, 4CB)
	DeutschePostLeitcode  = (1 << 27), ///< Deutsche Post Leitcode (14 digits, German routing)
	DeutschePostIdentcode = (1 << 28), ///< Deutsche Post Identcode (12 digits, German identification)
	Code11          = (1 << 29), ///< Code 11 (USD-8, telecommunications)
	POSTNET         = (1 << 30), ///< USPS POSTNET (Postal Numeric Encoding Technique)
	PLANET          = (1ull << 31), ///< USPS PLANET (Postal Alpha Numeric Encoding Technique)
	MSI             = (1ull << 32), ///< MSI (Modified Plessey, inventory/warehousing)
	Telepen         = (1ull << 33), ///< Telepen (Full ASCII, developed by SB Electronic Systems)
	LOGMARS         = (1ull << 34), ///< LOGMARS (Code 39 variant for US military, MIL-STD-1189)
	Code32          = (1ull << 35), ///< Code 32 (Italian Pharmacode, Code 39 variant for pharmaceuticals)
	Pharmacode      = (1ull << 36), ///< Pharmacode (Laetus, pharmaceutical binary barcode)
	PharmacodeTwoTrack = (1ull << 37), ///< Pharmacode Two-Track (Laetus, 3-state pharmaceutical barcode)
	PZN             = (1ull << 38), ///< Pharmazentralnummer (German pharmaceutical, Code 39 variant)
	ChannelCode     = (1ull << 39), ///< Channel Code (ANSI/AIM BC12, compact numeric encoding)
	Matrix2of5      = (1ull << 40), ///< Matrix 2 of 5 (Standard 2 of 5, discrete numeric encoding)
	Industrial2of5  = (1ull << 41), ///< Industrial 2 of 5 (Standard 2 of 5, bars-only encoding)

	LinearCodes = Codabar | Code39 | Code93 | Code128 | EAN8 | EAN13 | ITF | DataBar | DataBarExpanded | DataBarLimited
				  | DXFilmEdge | UPCA | UPCE | AustraliaPost | KIXCode | JapanPost | KoreaPost | RM4SCC | Mailmark | USPSIMB
				  | DeutschePostLeitcode | DeutschePostIdentcode | Code11 | POSTNET | PLANET | MSI | Telepen | LOGMARS | Code32
				  | Pharmacode | PharmacodeTwoTrack | PZN | ChannelCode | Matrix2of5 | Industrial2of5,
	MatrixCodes = Aztec | DataMatrix | MaxiCode | PDF417 | QRCode | MicroQRCode | RMQRCode,
	Any         = LinearCodes | MatrixCodes,

	_max = Industrial2of5, ///> implementation detail, don't use
};

ZX_DECLARE_FLAGS(BarcodeFormats, BarcodeFormat)

inline constexpr bool IsLinearBarcode(BarcodeFormat format)
{
	return BarcodeFormats(BarcodeFormat::LinearCodes).testFlag(format);
}

std::string ToString(BarcodeFormat format);
std::string ToString(BarcodeFormats formats);

/**
 * @brief Parse a string into a BarcodeFormat. '-' and '_' are optional.
 * @return None if str can not be parsed as a valid enum value
 */
BarcodeFormat BarcodeFormatFromString(std::string_view str);

/**
 * @brief Parse a string into a set of BarcodeFormats.
 * Separators can be (any combination of) '|', ',' or ' '.
 * Underscores are optional and input can be lower case.
 * e.g. "EAN-8 qrcode, Itf" would be parsed into [EAN8, QRCode, ITF].
 * @throws std::invalid_parameter if the string can not be fully parsed.
 */
BarcodeFormats BarcodeFormatsFromString(std::string_view str);

} // ZXing
