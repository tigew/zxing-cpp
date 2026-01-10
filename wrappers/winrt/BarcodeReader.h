/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "BarcodeFormat.h"
#include "ReaderOptions.h"

#include <memory>

namespace ZXing {

public enum class BarcodeType : int {
	AZTEC,
	AZTEC_RUNE,
	CODABAR,
	CODE_11,
	CODE_39,
	CODE_93,
	CODE_128,
	CODE_ONE,
	DATA_BAR,
	DATA_BAR_EXPANDED,
	DATA_BAR_LIMITED,
	DATA_BAR_STACKED,
	DATA_BAR_STACKED_OMNIDIRECTIONAL,
	DATA_BAR_EXPANDED_STACKED,
	DATA_MATRIX,
	DOT_CODE,
	GRID_MATRIX,
	HAN_XIN,
	DX_FILM_EDGE,
	EAN_8,
	EAN_13,
	ITF,
	AUSTRALIA_POST,
	JAPAN_POST,
	KIX_CODE,
	KOREA_POST,
	MAILMARK,
	RM4SCC,
	USPS_IMB,
	DEUTSCHE_POST_LEITCODE,
	DEUTSCHE_POST_IDENTCODE,
	POSTNET,
	PLANET,
	MSI,
	TELEPEN,
	LOGMARS,
	CODE_32,
	PHARMACODE,
	PHARMACODE_TWO_TRACK,
	PZN,
	CHANNEL_CODE,
	MATRIX_2_OF_5,
	INDUSTRIAL_2_OF_5,
	IATA_2_OF_5,
	DATALOGIC_2_OF_5,
	CODABLOCK_F,
	CODE_16K,
	CODE_49,
	MAXICODE,
	PDF_417,
	QR_CODE,
	MICRO_QR_CODE,
	RMQR_CODE,
	UPN_QR,
	UPC_A,
	UPC_E
};

ref class ReadResult;

public ref class BarcodeReader sealed
{
public:
	BarcodeReader(bool tryHarder, bool tryRotate, const Platform::Array<BarcodeType>^ types);
	BarcodeReader(bool tryHarder, bool tryRotate);
	BarcodeReader(bool tryHarder);

	ReadResult^ Read(Windows::Graphics::Imaging::SoftwareBitmap^ bitmap, int cropWidth, int cropHeight);

private:
	~BarcodeReader();

	void init(bool tryHarder, bool tryRotate, const Platform::Array<BarcodeType>^ types);

	static BarcodeFormat ConvertRuntimeToNative(BarcodeType type);
	static BarcodeType ConvertNativeToRuntime(BarcodeFormat format);

	std::unique_ptr<ReaderOptions> m_opts;
};

} // ZXing
