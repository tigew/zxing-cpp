/*
* Copyright 2024 ISNing
*/
// SPDX-License-Identifier: Apache-2.0

package zxingcpp

import kotlinx.cinterop.ExperimentalForeignApi
import zxingcpp.cinterop.*

@OptIn(ExperimentalForeignApi::class)
enum class BarcodeFormat(internal val rawValue: ULong) {
	None(ZXing_BarcodeFormat_None),
	Aztec(ZXing_BarcodeFormat_Aztec),
	AztecRune(ZXing_BarcodeFormat_AztecRune),
	Codabar(ZXing_BarcodeFormat_Codabar),
	Code11(ZXing_BarcodeFormat_Code11),
	Code39(ZXing_BarcodeFormat_Code39),
	Code93(ZXing_BarcodeFormat_Code93),
	Code128(ZXing_BarcodeFormat_Code128),
	CodeOne(ZXing_BarcodeFormat_CodeOne),
	DataBar(ZXing_BarcodeFormat_DataBar),
	DataBarExpanded(ZXing_BarcodeFormat_DataBarExpanded),
	DataBarLimited(ZXing_BarcodeFormat_DataBarLimited),
	DataBarStacked(ZXing_BarcodeFormat_DataBarStacked),
	DataBarStackedOmnidirectional(ZXing_BarcodeFormat_DataBarStackedOmnidirectional),
	DataBarExpandedStacked(ZXing_BarcodeFormat_DataBarExpandedStacked),
	DataMatrix(ZXing_BarcodeFormat_DataMatrix),
	DotCode(ZXing_BarcodeFormat_DotCode),
	GridMatrix(ZXing_BarcodeFormat_GridMatrix),
	HanXin(ZXing_BarcodeFormat_HanXin),
	DXFilmEdge(ZXing_BarcodeFormat_DXFilmEdge),
	EAN8(ZXing_BarcodeFormat_EAN8),
	EAN13(ZXing_BarcodeFormat_EAN13),
	ITF(ZXing_BarcodeFormat_ITF),
	AustraliaPost(ZXing_BarcodeFormat_AustraliaPost),
	JapanPost(ZXing_BarcodeFormat_JapanPost),
	KIXCode(ZXing_BarcodeFormat_KIXCode),
	KoreaPost(ZXing_BarcodeFormat_KoreaPost),
	Mailmark(ZXing_BarcodeFormat_Mailmark),
	RM4SCC(ZXing_BarcodeFormat_RM4SCC),
	USPSIMB(ZXing_BarcodeFormat_USPSIMB),
	DeutschePostLeitcode(ZXing_BarcodeFormat_DeutschePostLeitcode),
	DeutschePostIdentcode(ZXing_BarcodeFormat_DeutschePostIdentcode),
	POSTNET(ZXing_BarcodeFormat_POSTNET),
	PLANET(ZXing_BarcodeFormat_PLANET),
	MSI(ZXing_BarcodeFormat_MSI),
	Telepen(ZXing_BarcodeFormat_Telepen),
	LOGMARS(ZXing_BarcodeFormat_LOGMARS),
	Code32(ZXing_BarcodeFormat_Code32),
	Pharmacode(ZXing_BarcodeFormat_Pharmacode),
	PharmacodeTwoTrack(ZXing_BarcodeFormat_PharmacodeTwoTrack),
	PZN(ZXing_BarcodeFormat_PZN),
	ChannelCode(ZXing_BarcodeFormat_ChannelCode),
	Matrix2of5(ZXing_BarcodeFormat_Matrix2of5),
	Industrial2of5(ZXing_BarcodeFormat_Industrial2of5),
	IATA2of5(ZXing_BarcodeFormat_IATA2of5),
	Datalogic2of5(ZXing_BarcodeFormat_Datalogic2of5),
	CodablockF(ZXing_BarcodeFormat_CodablockF),
	Code16K(ZXing_BarcodeFormat_Code16K),
	Code49(ZXing_BarcodeFormat_Code49),
	MaxiCode(ZXing_BarcodeFormat_MaxiCode),
	PDF417(ZXing_BarcodeFormat_PDF417),
	QRCode(ZXing_BarcodeFormat_QRCode),
	MicroQRCode(ZXing_BarcodeFormat_MicroQRCode),
	RMQRCode(ZXing_BarcodeFormat_RMQRCode),
	UPNQR(ZXing_BarcodeFormat_UPNQR),
	UPCA(ZXing_BarcodeFormat_UPCA),
	UPCE(ZXing_BarcodeFormat_UPCE),

	LinearCodes(ZXing_BarcodeFormat_LinearCodes),
	MatrixCodes(ZXing_BarcodeFormat_MatrixCodes),
	Any(ZXing_BarcodeFormat_Any),

	Invalid(ZXing_BarcodeFormat_Invalid),
}

@OptIn(ExperimentalForeignApi::class)
fun ZXing_BarcodeFormat.parseIntoBarcodeFormat(): Set<BarcodeFormat> =
	BarcodeFormat.entries.filter { this.or(it.rawValue) == this }.toSet()

@OptIn(ExperimentalForeignApi::class)
fun Iterable<BarcodeFormat>.toValue(): ZXing_BarcodeFormat =
	this.map { it.rawValue }.reduce { acc, format -> acc.or(format) }
