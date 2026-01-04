/*
* Copyright 2023 siiky
* Copyright 2023 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#ifndef _ZXING_C_H
#define _ZXING_C_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus

#include "ZXingCpp.h"

typedef ZXing::Barcode ZXing_Barcode;
typedef ZXing::Barcodes ZXing_Barcodes;
typedef ZXing::ImageView ZXing_ImageView;
typedef ZXing::Image ZXing_Image;
typedef ZXing::ReaderOptions ZXing_ReaderOptions;

#ifdef ZXING_EXPERIMENTAL_API
typedef ZXing::CreatorOptions ZXing_CreatorOptions;
typedef ZXing::WriterOptions ZXing_WriterOptions;
#endif

extern "C"
{
#else

typedef struct ZXing_Barcode ZXing_Barcode;
typedef struct ZXing_Barcodes ZXing_Barcodes;
typedef struct ZXing_ImageView ZXing_ImageView;
typedef struct ZXing_Image ZXing_Image;
typedef struct ZXing_ReaderOptions ZXing_ReaderOptions;
typedef struct ZXing_CreatorOptions ZXing_CreatorOptions;
typedef struct ZXing_WriterOptions ZXing_WriterOptions;

#endif

/*
 * ZXing/ImageView.h
 */

typedef enum {
	ZXing_ImageFormat_None = 0,
	ZXing_ImageFormat_Lum = 0x01000000,
	ZXing_ImageFormat_LumA = 0x02000000,
	ZXing_ImageFormat_RGB = 0x03000102,
	ZXing_ImageFormat_BGR = 0x03020100,
	ZXing_ImageFormat_RGBA = 0x04000102,
	ZXing_ImageFormat_ARGB = 0x04010203,
	ZXing_ImageFormat_BGRA = 0x04020100,
	ZXing_ImageFormat_ABGR = 0x04030201,
} ZXing_ImageFormat;

ZXing_ImageView* ZXing_ImageView_new(const uint8_t* data, int width, int height, ZXing_ImageFormat format, int rowStride,
									 int pixStride);
ZXing_ImageView* ZXing_ImageView_new_checked(const uint8_t* data, int size, int width, int height, ZXing_ImageFormat format,
											 int rowStride, int pixStride);
void ZXing_ImageView_delete(ZXing_ImageView* iv);

void ZXing_ImageView_crop(ZXing_ImageView* iv, int left, int top, int width, int height);
void ZXing_ImageView_rotate(ZXing_ImageView* iv, int degree);

void ZXing_Image_delete(ZXing_Image* img);

const uint8_t* ZXing_Image_data(const ZXing_Image* img);
int ZXing_Image_width(const ZXing_Image* img);
int ZXing_Image_height(const ZXing_Image* img);
ZXing_ImageFormat ZXing_Image_format(const ZXing_Image* img);

/*
 * ZXing/BarcodeFormat.h
 */

typedef uint64_t ZXing_BarcodeFormat;

#define ZXing_BarcodeFormat_None            ((ZXing_BarcodeFormat)0)
#define ZXing_BarcodeFormat_Aztec           ((ZXing_BarcodeFormat)(1ull << 0))
#define ZXing_BarcodeFormat_Codabar         ((ZXing_BarcodeFormat)(1ull << 1))
#define ZXing_BarcodeFormat_Code39          ((ZXing_BarcodeFormat)(1ull << 2))
#define ZXing_BarcodeFormat_Code93          ((ZXing_BarcodeFormat)(1ull << 3))
#define ZXing_BarcodeFormat_Code128         ((ZXing_BarcodeFormat)(1ull << 4))
#define ZXing_BarcodeFormat_DataBar         ((ZXing_BarcodeFormat)(1ull << 5))
#define ZXing_BarcodeFormat_DataBarExpanded ((ZXing_BarcodeFormat)(1ull << 6))
#define ZXing_BarcodeFormat_DataMatrix      ((ZXing_BarcodeFormat)(1ull << 7))
#define ZXing_BarcodeFormat_EAN8            ((ZXing_BarcodeFormat)(1ull << 8))
#define ZXing_BarcodeFormat_EAN13           ((ZXing_BarcodeFormat)(1ull << 9))
#define ZXing_BarcodeFormat_ITF             ((ZXing_BarcodeFormat)(1ull << 10))
#define ZXing_BarcodeFormat_MaxiCode        ((ZXing_BarcodeFormat)(1ull << 11))
#define ZXing_BarcodeFormat_PDF417          ((ZXing_BarcodeFormat)(1ull << 12))
#define ZXing_BarcodeFormat_QRCode          ((ZXing_BarcodeFormat)(1ull << 13))
#define ZXing_BarcodeFormat_UPCA            ((ZXing_BarcodeFormat)(1ull << 14))
#define ZXing_BarcodeFormat_UPCE            ((ZXing_BarcodeFormat)(1ull << 15))
#define ZXing_BarcodeFormat_MicroQRCode     ((ZXing_BarcodeFormat)(1ull << 16))
#define ZXing_BarcodeFormat_RMQRCode        ((ZXing_BarcodeFormat)(1ull << 17))
#define ZXing_BarcodeFormat_DXFilmEdge      ((ZXing_BarcodeFormat)(1ull << 18))
#define ZXing_BarcodeFormat_DataBarLimited  ((ZXing_BarcodeFormat)(1ull << 19))
#define ZXing_BarcodeFormat_AustraliaPost   ((ZXing_BarcodeFormat)(1ull << 20))
#define ZXing_BarcodeFormat_KIXCode         ((ZXing_BarcodeFormat)(1ull << 21))
#define ZXing_BarcodeFormat_JapanPost       ((ZXing_BarcodeFormat)(1ull << 22))
#define ZXing_BarcodeFormat_KoreaPost       ((ZXing_BarcodeFormat)(1ull << 23))
#define ZXing_BarcodeFormat_RM4SCC          ((ZXing_BarcodeFormat)(1ull << 24))
#define ZXing_BarcodeFormat_Mailmark        ((ZXing_BarcodeFormat)(1ull << 25))
#define ZXing_BarcodeFormat_USPSIMB         ((ZXing_BarcodeFormat)(1ull << 26))
#define ZXing_BarcodeFormat_DeutschePostLeitcode  ((ZXing_BarcodeFormat)(1ull << 27))
#define ZXing_BarcodeFormat_DeutschePostIdentcode ((ZXing_BarcodeFormat)(1ull << 28))
#define ZXing_BarcodeFormat_Code11          ((ZXing_BarcodeFormat)(1ull << 29))
#define ZXing_BarcodeFormat_POSTNET         ((ZXing_BarcodeFormat)(1ull << 30))
#define ZXing_BarcodeFormat_PLANET          ((ZXing_BarcodeFormat)(1ull << 31))
#define ZXing_BarcodeFormat_MSI             ((ZXing_BarcodeFormat)(1ull << 32))
#define ZXing_BarcodeFormat_Telepen         ((ZXing_BarcodeFormat)(1ull << 33))
#define ZXing_BarcodeFormat_LOGMARS         ((ZXing_BarcodeFormat)(1ull << 34))
#define ZXing_BarcodeFormat_Code32          ((ZXing_BarcodeFormat)(1ull << 35))
#define ZXing_BarcodeFormat_Pharmacode      ((ZXing_BarcodeFormat)(1ull << 36))
#define ZXing_BarcodeFormat_PharmacodeTwoTrack ((ZXing_BarcodeFormat)(1ull << 37))
#define ZXing_BarcodeFormat_PZN             ((ZXing_BarcodeFormat)(1ull << 38))
#define ZXing_BarcodeFormat_ChannelCode     ((ZXing_BarcodeFormat)(1ull << 39))
#define ZXing_BarcodeFormat_Matrix2of5      ((ZXing_BarcodeFormat)(1ull << 40))
#define ZXing_BarcodeFormat_Industrial2of5  ((ZXing_BarcodeFormat)(1ull << 41))

#define ZXing_BarcodeFormat_LinearCodes (ZXing_BarcodeFormat_Codabar | ZXing_BarcodeFormat_Code39 | ZXing_BarcodeFormat_Code93 \
	| ZXing_BarcodeFormat_Code128 | ZXing_BarcodeFormat_EAN8 | ZXing_BarcodeFormat_EAN13 \
	| ZXing_BarcodeFormat_ITF | ZXing_BarcodeFormat_DataBar | ZXing_BarcodeFormat_DataBarExpanded \
	| ZXing_BarcodeFormat_DataBarLimited | ZXing_BarcodeFormat_DXFilmEdge | ZXing_BarcodeFormat_UPCA \
	| ZXing_BarcodeFormat_UPCE | ZXing_BarcodeFormat_AustraliaPost | ZXing_BarcodeFormat_KIXCode \
	| ZXing_BarcodeFormat_JapanPost | ZXing_BarcodeFormat_KoreaPost | ZXing_BarcodeFormat_RM4SCC \
	| ZXing_BarcodeFormat_Mailmark | ZXing_BarcodeFormat_USPSIMB | ZXing_BarcodeFormat_DeutschePostLeitcode \
	| ZXing_BarcodeFormat_DeutschePostIdentcode | ZXing_BarcodeFormat_Code11 \
	| ZXing_BarcodeFormat_POSTNET | ZXing_BarcodeFormat_PLANET | ZXing_BarcodeFormat_MSI \
	| ZXing_BarcodeFormat_Telepen | ZXing_BarcodeFormat_LOGMARS | ZXing_BarcodeFormat_Code32 \
	| ZXing_BarcodeFormat_Pharmacode | ZXing_BarcodeFormat_PharmacodeTwoTrack | ZXing_BarcodeFormat_PZN \
	| ZXing_BarcodeFormat_ChannelCode | ZXing_BarcodeFormat_Matrix2of5 | ZXing_BarcodeFormat_Industrial2of5)

#define ZXing_BarcodeFormat_MatrixCodes (ZXing_BarcodeFormat_Aztec | ZXing_BarcodeFormat_DataMatrix | ZXing_BarcodeFormat_MaxiCode \
	| ZXing_BarcodeFormat_PDF417 | ZXing_BarcodeFormat_QRCode | ZXing_BarcodeFormat_MicroQRCode \
	| ZXing_BarcodeFormat_RMQRCode)

#define ZXing_BarcodeFormat_Any (ZXing_BarcodeFormat_LinearCodes | ZXing_BarcodeFormat_MatrixCodes)

#define ZXing_BarcodeFormat_Invalid ((ZXing_BarcodeFormat)0xFFFFFFFFFFFFFFFFull)

typedef ZXing_BarcodeFormat ZXing_BarcodeFormats;

ZXing_BarcodeFormats ZXing_BarcodeFormatsFromString(const char* str);
ZXing_BarcodeFormat ZXing_BarcodeFormatFromString(const char* str);
char* ZXing_BarcodeFormatToString(ZXing_BarcodeFormat format);

/*
 * ZXing/ZXingCpp.h
 */

#ifdef ZXING_EXPERIMENTAL_API

typedef enum {
	ZXing_Operation_Create,
	ZXing_Operation_Read,
	ZXing_Operation_CreateAndRead,
	ZXing_Operation_CreateOrRead,
} ZXing_Operation;

ZXing_BarcodeFormats ZXing_SupportedBarcodeFormats(ZXing_Operation op);

#endif

/*
 * ZXing/Barcode.h
 */

typedef enum
{
	ZXing_ContentType_Text,
	ZXing_ContentType_Binary,
	ZXing_ContentType_Mixed,
	ZXing_ContentType_GS1,
	ZXing_ContentType_ISO15434,
	ZXing_ContentType_UnknownECI
} ZXing_ContentType;

typedef enum
{
	ZXing_ErrorType_None,
	ZXing_ErrorType_Format,
	ZXing_ErrorType_Checksum,
	ZXing_ErrorType_Unsupported
} ZXing_ErrorType;

char* ZXing_ContentTypeToString(ZXing_ContentType type);

typedef struct ZXing_PointI
{
	int x, y;
} ZXing_PointI;

typedef struct ZXing_Position
{
	ZXing_PointI topLeft, topRight, bottomRight, bottomLeft;
} ZXing_Position;

char* ZXing_PositionToString(ZXing_Position position);

bool ZXing_Barcode_isValid(const ZXing_Barcode* barcode);
ZXing_ErrorType ZXing_Barcode_errorType(const ZXing_Barcode* barcode);
char* ZXing_Barcode_errorMsg(const ZXing_Barcode* barcode);
ZXing_BarcodeFormat ZXing_Barcode_format(const ZXing_Barcode* barcode);
ZXing_ContentType ZXing_Barcode_contentType(const ZXing_Barcode* barcode);
uint8_t* ZXing_Barcode_bytes(const ZXing_Barcode* barcode, int* len);
uint8_t* ZXing_Barcode_bytesECI(const ZXing_Barcode* barcode, int* len);
char* ZXing_Barcode_text(const ZXing_Barcode* barcode);
char* ZXing_Barcode_ecLevel(const ZXing_Barcode* barcode);
char* ZXing_Barcode_symbologyIdentifier(const ZXing_Barcode* barcode);
ZXing_Position ZXing_Barcode_position(const ZXing_Barcode* barcode);
int ZXing_Barcode_orientation(const ZXing_Barcode* barcode);
bool ZXing_Barcode_hasECI(const ZXing_Barcode* barcode);
bool ZXing_Barcode_isInverted(const ZXing_Barcode* barcode);
bool ZXing_Barcode_isMirrored(const ZXing_Barcode* barcode);
int ZXing_Barcode_lineCount(const ZXing_Barcode* barcode);
int ZXing_Barcode_sequenceIndex(const ZXing_Barcode* barcode);
int ZXing_Barcode_sequenceSize(const ZXing_Barcode* barcode);
char* ZXing_Barcode_sequenceId(const ZXing_Barcode* barcode);

void ZXing_Barcode_delete(ZXing_Barcode* barcode);
void ZXing_Barcodes_delete(ZXing_Barcodes* barcodes);

int ZXing_Barcodes_size(const ZXing_Barcodes* barcodes);
const ZXing_Barcode* ZXing_Barcodes_at(const ZXing_Barcodes* barcodes, int i);
ZXing_Barcode* ZXing_Barcodes_move(ZXing_Barcodes* barcodes, int i);

/*
 * ZXing/ReaderOptions.h
 */

typedef enum
{
	ZXing_Binarizer_LocalAverage,
	ZXing_Binarizer_GlobalHistogram,
	ZXing_Binarizer_FixedThreshold,
	ZXing_Binarizer_BoolCast,
} ZXing_Binarizer;

typedef enum
{
	ZXing_EanAddOnSymbol_Ignore,
	ZXing_EanAddOnSymbol_Read,
	ZXing_EanAddOnSymbol_Require,
} ZXing_EanAddOnSymbol;

typedef enum
{
	ZXing_TextMode_Plain,
	ZXing_TextMode_ECI,
	ZXing_TextMode_HRI,
	ZXing_TextMode_Hex,
	ZXing_TextMode_Escaped,
} ZXing_TextMode;

ZXing_ReaderOptions* ZXing_ReaderOptions_new();
void ZXing_ReaderOptions_delete(ZXing_ReaderOptions* opts);

void ZXing_ReaderOptions_setTryHarder(ZXing_ReaderOptions* opts, bool tryHarder);
void ZXing_ReaderOptions_setTryRotate(ZXing_ReaderOptions* opts, bool tryRotate);
void ZXing_ReaderOptions_setTryInvert(ZXing_ReaderOptions* opts, bool tryInvert);
void ZXing_ReaderOptions_setTryDownscale(ZXing_ReaderOptions* opts, bool tryDownscale);
#ifdef ZXING_EXPERIMENTAL_API
	void ZXing_ReaderOptions_setTryDenoise(ZXing_ReaderOptions* opts, bool tryDenoise);
#endif
void ZXing_ReaderOptions_setIsPure(ZXing_ReaderOptions* opts, bool isPure);
void ZXing_ReaderOptions_setReturnErrors(ZXing_ReaderOptions* opts, bool returnErrors);
void ZXing_ReaderOptions_setFormats(ZXing_ReaderOptions* opts, ZXing_BarcodeFormats formats);
void ZXing_ReaderOptions_setBinarizer(ZXing_ReaderOptions* opts, ZXing_Binarizer binarizer);
void ZXing_ReaderOptions_setEanAddOnSymbol(ZXing_ReaderOptions* opts, ZXing_EanAddOnSymbol eanAddOnSymbol);
void ZXing_ReaderOptions_setTextMode(ZXing_ReaderOptions* opts, ZXing_TextMode textMode);
void ZXing_ReaderOptions_setMinLineCount(ZXing_ReaderOptions* opts, int n);
void ZXing_ReaderOptions_setMaxNumberOfSymbols(ZXing_ReaderOptions* opts, int n);

bool ZXing_ReaderOptions_getTryHarder(const ZXing_ReaderOptions* opts);
bool ZXing_ReaderOptions_getTryRotate(const ZXing_ReaderOptions* opts);
bool ZXing_ReaderOptions_getTryInvert(const ZXing_ReaderOptions* opts);
bool ZXing_ReaderOptions_getTryDownscale(const ZXing_ReaderOptions* opts);
#ifdef ZXING_EXPERIMENTAL_API
	bool ZXing_ReaderOptions_getTryDenoise(const ZXing_ReaderOptions* opts);
#endif
bool ZXing_ReaderOptions_getIsPure(const ZXing_ReaderOptions* opts);
bool ZXing_ReaderOptions_getReturnErrors(const ZXing_ReaderOptions* opts);
ZXing_BarcodeFormats ZXing_ReaderOptions_getFormats(const ZXing_ReaderOptions* opts);
ZXing_Binarizer ZXing_ReaderOptions_getBinarizer(const ZXing_ReaderOptions* opts);
ZXing_EanAddOnSymbol ZXing_ReaderOptions_getEanAddOnSymbol(const ZXing_ReaderOptions* opts);
ZXing_TextMode ZXing_ReaderOptions_getTextMode(const ZXing_ReaderOptions* opts);
int ZXing_ReaderOptions_getMinLineCount(const ZXing_ReaderOptions* opts);
int ZXing_ReaderOptions_getMaxNumberOfSymbols(const ZXing_ReaderOptions* opts);

/*
 * ZXing/ReadBarcode.h
 */

/** Note: opts is optional, i.e. it can be NULL, which will imply default settings. */
ZXing_Barcodes* ZXing_ReadBarcodes(const ZXing_ImageView* iv, const ZXing_ReaderOptions* opts);

#ifdef ZXING_EXPERIMENTAL_API

/*
 * ZXing/WriteBarcode.h
 */

ZXing_CreatorOptions* ZXing_CreatorOptions_new(ZXing_BarcodeFormat format);
void ZXing_CreatorOptions_delete(ZXing_CreatorOptions* opts);

void ZXing_CreatorOptions_setFormat(ZXing_CreatorOptions* opts, ZXing_BarcodeFormat format);
ZXing_BarcodeFormat ZXing_CreatorOptions_getFormat(const ZXing_CreatorOptions* opts);

void ZXing_CreatorOptions_setOptions(ZXing_CreatorOptions* opts, const char* options);
char* ZXing_CreatorOptions_getOptions(const ZXing_CreatorOptions* opts);


ZXing_WriterOptions* ZXing_WriterOptions_new();
void ZXing_WriterOptions_delete(ZXing_WriterOptions* opts);

void ZXing_WriterOptions_setScale(ZXing_WriterOptions* opts, int scale);
int ZXing_WriterOptions_getScale(const ZXing_WriterOptions* opts);

void ZXing_WriterOptions_setSizeHint(ZXing_WriterOptions* opts, int sizeHint);
int ZXing_WriterOptions_getSizeHint(const ZXing_WriterOptions* opts);

void ZXing_WriterOptions_setRotate(ZXing_WriterOptions* opts, int rotate);
int ZXing_WriterOptions_getRotate(const ZXing_WriterOptions* opts);

void ZXing_WriterOptions_setAddHRT(ZXing_WriterOptions* opts, bool addHRT);
bool ZXing_WriterOptions_getAddHRT(const ZXing_WriterOptions* opts);

void ZXing_WriterOptions_setAddQuietZones(ZXing_WriterOptions* opts, bool addQuietZones);
bool ZXing_WriterOptions_getAddQuietZones(const ZXing_WriterOptions* opts);


ZXing_Barcode* ZXing_CreateBarcodeFromText(const char* data, int size, const ZXing_CreatorOptions* opts);
ZXing_Barcode* ZXing_CreateBarcodeFromBytes(const void* data, int size, const ZXing_CreatorOptions* opts);

/** Note: opts is optional, i.e. it can be NULL, which will imply default settings. */
char* ZXing_WriteBarcodeToSVG(const ZXing_Barcode* barcode, const ZXing_WriterOptions* opts);
ZXing_Image* ZXing_WriteBarcodeToImage(const ZXing_Barcode* barcode, const ZXing_WriterOptions* opts);

#endif /* ZXING_EXPERIMENTAL_API */

/* ZXing_LastErrorMsg() returns NULL in case there is no last error and a copy of the string otherwise. */
char* ZXing_LastErrorMsg();

const char* ZXing_Version();

void ZXing_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* _ZXING_C_H */
