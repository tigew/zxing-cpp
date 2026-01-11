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
	AustraliaPost   = (1 << 0),  ///< Australia Post 4-State (Standard Customer, Reply Paid, Routing, Redirection)
	Aztec           = (1 << 1),  ///< Aztec
	AztecRune       = (1 << 2),  ///< Aztec Rune (compact 11x11 Aztec variant encoding 0-255)
	ChannelCode     = (1 << 3),  ///< Channel Code (ANSI/AIM BC12, compact numeric encoding)
	Codabar         = (1 << 4),  ///< Codabar
	CodablockF      = (1 << 5),  ///< Codablock F (stacked Code 128, 2-44 rows)
	Code11          = (1 << 6),  ///< Code 11 (USD-8, telecommunications)
	Code128         = (1 << 7),  ///< Code128
	Code16K         = (1 << 8),  ///< Code 16K (stacked Code 128, 2-16 rows)
	Code32          = (1 << 9),  ///< Code 32 (Italian Pharmacode, Code 39 variant for pharmaceuticals)
	Code39          = (1 << 10), ///< Code39
	Code49          = (1 << 11), ///< Code 49 (USS-49, stacked 2-8 rows, full ASCII)
	Code93          = (1 << 12), ///< Code93
	CodeOne         = (1 << 13), ///< Code One (2D matrix, versions A-H/S/T, AIM USS-Code One)
	DataBar         = (1 << 14), ///< GS1 DataBar, formerly known as RSS 14
	DataBarExpanded = (1 << 15), ///< GS1 DataBar Expanded, formerly known as RSS EXPANDED
	DataBarExpandedStacked = (1 << 16), ///< GS1 DataBar Expanded Stacked (multi-row variant)
	DataBarLimited  = (1 << 17), ///< GS1 DataBar Limited
	DataBarStacked  = (1 << 18), ///< GS1 DataBar Stacked (RSS-14 Stacked, 2-row variant)
	DataBarStackedOmnidirectional = (1 << 19), ///< GS1 DataBar Stacked Omnidirectional (RSS-14 Stacked Omni)
	Datalogic2of5   = (1 << 20), ///< Datalogic 2 of 5 (China Post, Code 2 of 5 Data Logic)
	DataMatrix      = (1 << 21), ///< DataMatrix
	DeutschePostIdentcode = (1 << 22), ///< Deutsche Post Identcode (12 digits, German identification)
	DeutschePostLeitcode  = (1 << 23), ///< Deutsche Post Leitcode (14 digits, German routing)
	DotCode         = (1 << 24), ///< DotCode (2D dot matrix, high-speed industrial printing)
	DXFilmEdge      = (1 << 25), ///< DX Film Edge Barcode
	EAN13           = (1 << 26), ///< EAN-13
	EAN8            = (1 << 27), ///< EAN-8
	GridMatrix      = (1 << 28), ///< Grid Matrix (2D matrix, Chinese characters, GB/T 21049)
	HanXin          = (1 << 29), ///< Han Xin Code (2D matrix, Chinese standard GB/T 21049, ISO/IEC 20830)
	IATA2of5        = (1 << 30), ///< IATA 2 of 5 (Airline 2 of 5, air cargo management)
	Industrial2of5  = (1ull << 31), ///< Industrial 2 of 5 (Standard 2 of 5, bars-only encoding)
	ITF             = (1ull << 32), ///< ITF (Interleaved Two of Five)
	JapanPost       = (1ull << 33), ///< Japan Post 4-State Customer Code (Kasutama Barcode)
	KIXCode         = (1ull << 34), ///< Dutch Post KIX Code 4-State (Netherlands postal)
	KoreaPost       = (1ull << 35), ///< Korea Post Barcode (Korean Postal Authority Code)
	LOGMARS         = (1ull << 36), ///< LOGMARS (Code 39 variant for US military, MIL-STD-1189)
	Mailmark        = (1ull << 37), ///< Royal Mail 4-State Mailmark (UK postal, Types C and L)
	Matrix2of5      = (1ull << 38), ///< Matrix 2 of 5 (Standard 2 of 5, discrete numeric encoding)
	MaxiCode        = (1ull << 39), ///< MaxiCode
	MicroQRCode     = (1ull << 40), ///< Micro QR Code
	MSI             = (1ull << 41), ///< MSI (Modified Plessey, inventory/warehousing)
	PDF417          = (1ull << 42), ///< PDF417
	Pharmacode      = (1ull << 43), ///< Pharmacode (Laetus, pharmaceutical binary barcode)
	PharmacodeTwoTrack = (1ull << 44), ///< Pharmacode Two-Track (Laetus, 3-state pharmaceutical barcode)
	PLANET          = (1ull << 45), ///< USPS PLANET (Postal Alpha Numeric Encoding Technique)
	POSTNET         = (1ull << 46), ///< USPS POSTNET (Postal Numeric Encoding Technique)
	PZN             = (1ull << 47), ///< Pharmazentralnummer (German pharmaceutical, Code 39 variant)
	QRCode          = (1ull << 48), ///< QR Code
	RM4SCC          = (1ull << 49), ///< Royal Mail 4-State Customer Code (UK postal)
	RMQRCode        = (1ull << 50), ///< Rectangular Micro QR Code
	Telepen         = (1ull << 51), ///< Telepen (Full ASCII, developed by SB Electronic Systems)
	UPCA            = (1ull << 52), ///< UPC-A
	UPCE            = (1ull << 53), ///< UPC-E
	UPNQR           = (1ull << 54), ///< UPN QR Code (Slovenian payment QR, Version 15, ECI 4, EC level M)
	USPSIMB         = (1ull << 55), ///< USPS Intelligent Mail Barcode (OneCode, 4CB)

	LinearCodes = Codabar | Code39 | Code93 | Code128 | EAN8 | EAN13 | ITF | DataBar | DataBarExpanded | DataBarLimited
				  | DataBarStacked | DataBarStackedOmnidirectional | DataBarExpandedStacked
				  | DXFilmEdge | UPCA | UPCE | AustraliaPost | KIXCode | JapanPost | KoreaPost | RM4SCC | Mailmark | USPSIMB
				  | DeutschePostLeitcode | DeutschePostIdentcode | Code11 | POSTNET | PLANET | MSI | Telepen | LOGMARS | Code32
				  | Pharmacode | PharmacodeTwoTrack | PZN | ChannelCode | Matrix2of5 | Industrial2of5 | IATA2of5 | Datalogic2of5
				  | CodablockF | Code16K | Code49,
	MatrixCodes = Aztec | AztecRune | CodeOne | DataMatrix | DotCode | GridMatrix | HanXin | MaxiCode | PDF417 | QRCode | MicroQRCode | RMQRCode | UPNQR,
	Any         = LinearCodes | MatrixCodes,

	_max = USPSIMB, ///> implementation detail, don't use
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
