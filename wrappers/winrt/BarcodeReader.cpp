/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#if (_MSC_VER >= 1915)
#define no_init_all deprecated
#pragma warning(disable : 4996)
#endif

#include "BarcodeReader.h"

#include "BarcodeFormat.h"
#include "ReaderOptions.h"
#include "ReadBarcode.h"
#include "ReadResult.h"
#include "Utf.h"

#include <algorithm>
#include <MemoryBuffer.h>
#include <stdexcept>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Imaging;

namespace ZXing {

BarcodeReader::BarcodeReader(bool tryHarder, bool tryRotate, const Platform::Array<BarcodeType>^ types)
{
	init(tryHarder, tryRotate, types);
}

BarcodeReader::BarcodeReader(bool tryHarder, bool tryRotate)
{
	init(tryHarder, tryRotate, nullptr);
}

BarcodeReader::BarcodeReader(bool tryHarder)
{
	init(tryHarder, tryHarder, nullptr);
}

void
BarcodeReader::init(bool tryHarder, bool tryRotate, const Platform::Array<BarcodeType>^ types)
{
	m_opts.reset(new ReaderOptions());
	m_opts->setTryHarder(tryHarder);
	m_opts->setTryRotate(tryRotate);
	m_opts->setTryInvert(tryHarder);

	if (types != nullptr && types->Length > 0) {
		BarcodeFormats barcodeFormats;
		for (BarcodeType type : types) {
			barcodeFormats |= BarcodeReader::ConvertRuntimeToNative(type);
		}
		m_opts->setFormats(barcodeFormats);
	}
}

BarcodeReader::~BarcodeReader()
{
}

BarcodeFormat BarcodeReader::ConvertRuntimeToNative(BarcodeType type)
{
	switch (type) {
	case BarcodeType::AZTEC: return BarcodeFormat::Aztec;
	case BarcodeType::AZTEC_RUNE: return BarcodeFormat::AztecRune;
	case BarcodeType::CODABAR: return BarcodeFormat::Codabar;
	case BarcodeType::CODE_11: return BarcodeFormat::Code11;
	case BarcodeType::CODE_39: return BarcodeFormat::Code39;
	case BarcodeType::CODE_93: return BarcodeFormat::Code93;
	case BarcodeType::CODE_128: return BarcodeFormat::Code128;
	case BarcodeType::CODE_ONE: return BarcodeFormat::CodeOne;
	case BarcodeType::DATA_BAR: return BarcodeFormat::DataBar;
	case BarcodeType::DATA_BAR_EXPANDED: return BarcodeFormat::DataBarExpanded;
	case BarcodeType::DATA_BAR_LIMITED: return BarcodeFormat::DataBarLimited;
	case BarcodeType::DATA_BAR_STACKED: return BarcodeFormat::DataBarStacked;
	case BarcodeType::DATA_BAR_STACKED_OMNIDIRECTIONAL: return BarcodeFormat::DataBarStackedOmnidirectional;
	case BarcodeType::DATA_BAR_EXPANDED_STACKED: return BarcodeFormat::DataBarExpandedStacked;
	case BarcodeType::DATA_MATRIX: return BarcodeFormat::DataMatrix;
	case BarcodeType::DOT_CODE: return BarcodeFormat::DotCode;
	case BarcodeType::GRID_MATRIX: return BarcodeFormat::GridMatrix;
	case BarcodeType::HAN_XIN: return BarcodeFormat::HanXin;
	case BarcodeType::DX_FILM_EDGE: return BarcodeFormat::DXFilmEdge;
	case BarcodeType::EAN_8: return BarcodeFormat::EAN8;
	case BarcodeType::EAN_13: return BarcodeFormat::EAN13;
	case BarcodeType::ITF: return BarcodeFormat::ITF;
	case BarcodeType::AUSTRALIA_POST: return BarcodeFormat::AustraliaPost;
	case BarcodeType::JAPAN_POST: return BarcodeFormat::JapanPost;
	case BarcodeType::KIX_CODE: return BarcodeFormat::KIXCode;
	case BarcodeType::KOREA_POST: return BarcodeFormat::KoreaPost;
	case BarcodeType::MAILMARK: return BarcodeFormat::Mailmark;
	case BarcodeType::RM4SCC: return BarcodeFormat::RM4SCC;
	case BarcodeType::USPS_IMB: return BarcodeFormat::USPSIMB;
	case BarcodeType::DEUTSCHE_POST_LEITCODE: return BarcodeFormat::DeutschePostLeitcode;
	case BarcodeType::DEUTSCHE_POST_IDENTCODE: return BarcodeFormat::DeutschePostIdentcode;
	case BarcodeType::POSTNET: return BarcodeFormat::POSTNET;
	case BarcodeType::PLANET: return BarcodeFormat::PLANET;
	case BarcodeType::MSI: return BarcodeFormat::MSI;
	case BarcodeType::TELEPEN: return BarcodeFormat::Telepen;
	case BarcodeType::LOGMARS: return BarcodeFormat::LOGMARS;
	case BarcodeType::CODE_32: return BarcodeFormat::Code32;
	case BarcodeType::PHARMACODE: return BarcodeFormat::Pharmacode;
	case BarcodeType::PHARMACODE_TWO_TRACK: return BarcodeFormat::PharmacodeTwoTrack;
	case BarcodeType::PZN: return BarcodeFormat::PZN;
	case BarcodeType::CHANNEL_CODE: return BarcodeFormat::ChannelCode;
	case BarcodeType::MATRIX_2_OF_5: return BarcodeFormat::Matrix2of5;
	case BarcodeType::INDUSTRIAL_2_OF_5: return BarcodeFormat::Industrial2of5;
	case BarcodeType::IATA_2_OF_5: return BarcodeFormat::IATA2of5;
	case BarcodeType::DATALOGIC_2_OF_5: return BarcodeFormat::Datalogic2of5;
	case BarcodeType::CODABLOCK_F: return BarcodeFormat::CodablockF;
	case BarcodeType::CODE_16K: return BarcodeFormat::Code16K;
	case BarcodeType::CODE_49: return BarcodeFormat::Code49;
	case BarcodeType::MAXICODE: return BarcodeFormat::MaxiCode;
	case BarcodeType::PDF_417: return BarcodeFormat::PDF417;
	case BarcodeType::QR_CODE: return BarcodeFormat::QRCode;
	case BarcodeType::MICRO_QR_CODE: return BarcodeFormat::MicroQRCode;
	case BarcodeType::RMQR_CODE: return BarcodeFormat::RMQRCode;
	case BarcodeType::UPN_QR: return BarcodeFormat::UPNQR;
	case BarcodeType::UPC_A: return BarcodeFormat::UPCA;
	case BarcodeType::UPC_E: return BarcodeFormat::UPCE;
	default:
		std::wstring typeAsString = type.ToString()->Begin();
		throw std::invalid_argument("Unknown Barcode Type: " + ToUtf8(typeAsString));
	}
}

BarcodeType BarcodeReader::ConvertNativeToRuntime(BarcodeFormat format)
{
	switch (format) {
	case BarcodeFormat::Aztec: return BarcodeType::AZTEC;
	case BarcodeFormat::AztecRune: return BarcodeType::AZTEC_RUNE;
	case BarcodeFormat::Codabar: return BarcodeType::CODABAR;
	case BarcodeFormat::Code11: return BarcodeType::CODE_11;
	case BarcodeFormat::Code39: return BarcodeType::CODE_39;
	case BarcodeFormat::Code93: return BarcodeType::CODE_93;
	case BarcodeFormat::Code128: return BarcodeType::CODE_128;
	case BarcodeFormat::CodeOne: return BarcodeType::CODE_ONE;
	case BarcodeFormat::DataBar: return BarcodeType::DATA_BAR;
	case BarcodeFormat::DataBarExpanded: return BarcodeType::DATA_BAR_EXPANDED;
	case BarcodeFormat::DataBarLimited: return BarcodeType::DATA_BAR_LIMITED;
	case BarcodeFormat::DataBarStacked: return BarcodeType::DATA_BAR_STACKED;
	case BarcodeFormat::DataBarStackedOmnidirectional: return BarcodeType::DATA_BAR_STACKED_OMNIDIRECTIONAL;
	case BarcodeFormat::DataBarExpandedStacked: return BarcodeType::DATA_BAR_EXPANDED_STACKED;
	case BarcodeFormat::DataMatrix: return BarcodeType::DATA_MATRIX;
	case BarcodeFormat::DotCode: return BarcodeType::DOT_CODE;
	case BarcodeFormat::GridMatrix: return BarcodeType::GRID_MATRIX;
	case BarcodeFormat::HanXin: return BarcodeType::HAN_XIN;
	case BarcodeFormat::DXFilmEdge: return BarcodeType::DX_FILM_EDGE;
	case BarcodeFormat::EAN8: return BarcodeType::EAN_8;
	case BarcodeFormat::EAN13: return BarcodeType::EAN_13;
	case BarcodeFormat::ITF: return BarcodeType::ITF;
	case BarcodeFormat::AustraliaPost: return BarcodeType::AUSTRALIA_POST;
	case BarcodeFormat::JapanPost: return BarcodeType::JAPAN_POST;
	case BarcodeFormat::KIXCode: return BarcodeType::KIX_CODE;
	case BarcodeFormat::KoreaPost: return BarcodeType::KOREA_POST;
	case BarcodeFormat::Mailmark: return BarcodeType::MAILMARK;
	case BarcodeFormat::RM4SCC: return BarcodeType::RM4SCC;
	case BarcodeFormat::USPSIMB: return BarcodeType::USPS_IMB;
	case BarcodeFormat::DeutschePostLeitcode: return BarcodeType::DEUTSCHE_POST_LEITCODE;
	case BarcodeFormat::DeutschePostIdentcode: return BarcodeType::DEUTSCHE_POST_IDENTCODE;
	case BarcodeFormat::POSTNET: return BarcodeType::POSTNET;
	case BarcodeFormat::PLANET: return BarcodeType::PLANET;
	case BarcodeFormat::MSI: return BarcodeType::MSI;
	case BarcodeFormat::Telepen: return BarcodeType::TELEPEN;
	case BarcodeFormat::LOGMARS: return BarcodeType::LOGMARS;
	case BarcodeFormat::Code32: return BarcodeType::CODE_32;
	case BarcodeFormat::Pharmacode: return BarcodeType::PHARMACODE;
	case BarcodeFormat::PharmacodeTwoTrack: return BarcodeType::PHARMACODE_TWO_TRACK;
	case BarcodeFormat::PZN: return BarcodeType::PZN;
	case BarcodeFormat::ChannelCode: return BarcodeType::CHANNEL_CODE;
	case BarcodeFormat::Matrix2of5: return BarcodeType::MATRIX_2_OF_5;
	case BarcodeFormat::Industrial2of5: return BarcodeType::INDUSTRIAL_2_OF_5;
	case BarcodeFormat::IATA2of5: return BarcodeType::IATA_2_OF_5;
	case BarcodeFormat::Datalogic2of5: return BarcodeType::DATALOGIC_2_OF_5;
	case BarcodeFormat::CodablockF: return BarcodeType::CODABLOCK_F;
	case BarcodeFormat::Code16K: return BarcodeType::CODE_16K;
	case BarcodeFormat::Code49: return BarcodeType::CODE_49;
	case BarcodeFormat::MaxiCode: return BarcodeType::MAXICODE;
	case BarcodeFormat::PDF417: return BarcodeType::PDF_417;
	case BarcodeFormat::QRCode: return BarcodeType::QR_CODE;
	case BarcodeFormat::MicroQRCode: return BarcodeType::MICRO_QR_CODE;
	case BarcodeFormat::RMQRCode: return BarcodeType::RMQR_CODE;
	case BarcodeFormat::UPNQR: return BarcodeType::UPN_QR;
	case BarcodeFormat::UPCA: return BarcodeType::UPC_A;
	case BarcodeFormat::UPCE: return BarcodeType::UPC_E;
	default:
		throw std::invalid_argument("Unknown Barcode Format ");
	}
}

static Platform::String^ ToPlatformString(const std::string& str)
{
	std::wstring wstr = FromUtf8(str);
	return ref new Platform::String(wstr.c_str(), (unsigned)wstr.length());
}

ReadResult^
BarcodeReader::Read(SoftwareBitmap^ bitmap, int cropWidth, int cropHeight)
{
	try {
		cropWidth = cropWidth <= 0 ? bitmap->PixelWidth : std::min(bitmap->PixelWidth, cropWidth);
		cropHeight = cropHeight <= 0 ? bitmap->PixelHeight : std::min(bitmap->PixelHeight, cropHeight);
		int cropLeft = (bitmap->PixelWidth - cropWidth) / 2;
		int cropTop = (bitmap->PixelHeight - cropHeight) / 2;

		auto inBuffer = bitmap->LockBuffer(BitmapBufferAccessMode::Read);
		auto inMemRef = inBuffer->CreateReference();
		ComPtr<IMemoryBufferByteAccess> inBufferAccess;

		if (SUCCEEDED(ComPtr<IUnknown>(reinterpret_cast<IUnknown*>(inMemRef)).As(&inBufferAccess))) {
			BYTE* inBytes = nullptr;
			UINT32 inCapacity = 0;
			inBufferAccess->GetBuffer(&inBytes, &inCapacity);

			ImageFormat fmt = ImageFormat::None;
			switch (bitmap->BitmapPixelFormat)
			{
			case BitmapPixelFormat::Gray8: fmt = ImageFormat::Lum; break;
			case BitmapPixelFormat::Bgra8: fmt = ImageFormat::BGRA; break;
			case BitmapPixelFormat::Rgba8: fmt = ImageFormat::RGBA; break;
			default:
				throw std::runtime_error("Unsupported BitmapPixelFormat");
			}

			auto img = ImageView(inBytes, bitmap->PixelWidth, bitmap->PixelHeight, fmt, inBuffer->GetPlaneDescription(0).Stride)
						   .cropped(cropLeft, cropTop, cropWidth, cropHeight);

			auto barcode = ReadBarcode(img, *m_opts);
			if (barcode.isValid()) {
				return ref new ReadResult(ToPlatformString(ZXing::ToString(barcode.format())), ToPlatformString(barcode.text()), ConvertNativeToRuntime(barcode.format()));
			}
		} else {
			throw std::runtime_error("Failed to read bitmap's data");
		}
	}
	catch (const std::exception& e) {
		OutputDebugStringA(e.what());
	}
	catch (...) {
	}
	return nullptr;
}

} // ZXing
