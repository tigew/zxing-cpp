cmake_minimum_required(VERSION 3.16)

project (ZXing VERSION "2.3.0")
set (ZXING_SONAME 3) # see https://github.com/zxing-cpp/zxing-cpp/issues/333

option ("ZXING_ENABLE_ALL" "Enable all barcode families" ON)
option ("ZXING_ENABLE_CATEGORY_ONED" "Enable One-Dimensional symbols" ON)
option ("ZXING_ENABLE_CATEGORY_STACKED" "Enable Stacked symbologies" ON)
option ("ZXING_ENABLE_CATEGORY_COMPOSITE" "Enable GS1 composite symbols" ON)
option ("ZXING_ENABLE_CATEGORY_TWOTRACK" "Enable Two-Track symbols" ON)
option ("ZXING_ENABLE_CATEGORY_POSTAL" "Enable 4-State postal symbols" ON)
option ("ZXING_ENABLE_CATEGORY_MATRIX" "Enable Matrix symbologies" ON)
option ("ZXING_ENABLE_CATEGORY_OTHER" "Enable other barcode-like markings" ON)

foreach(format 1D AZTEC DATAMATRIX MAXICODE PDF417 QRCODE MICRO_QR_CODE RMQR_CODE AUSTRALIA_POST
               CODABAR CODE_39 CODE_93 CODE_128 DATA_BAR DATA_BAR_EXPANDED DATA_BAR_LIMITED DX_FILM_EDGE
               EAN_8 EAN_13 ITF UPC_A UPC_E)
    option ("ZXING_ENABLE_${format}" "Enable support for ${format} barcodes" ON)
endforeach()

function (zxing_apply_enable out_var category_var format_var)
    if (ZXING_ENABLE_ALL AND ${category_var} AND ${format_var})
        set (${out_var} ON PARENT_SCOPE)
    else()
        set (${out_var} OFF PARENT_SCOPE)
    endif()
endfunction()

if (NOT ZXING_ENABLE_ALL)
    set (ZXING_ENABLE_CATEGORY_ONED OFF)
    set (ZXING_ENABLE_CATEGORY_STACKED OFF)
    set (ZXING_ENABLE_CATEGORY_COMPOSITE OFF)
    set (ZXING_ENABLE_CATEGORY_TWOTRACK OFF)
    set (ZXING_ENABLE_CATEGORY_POSTAL OFF)
    set (ZXING_ENABLE_CATEGORY_MATRIX OFF)
    set (ZXING_ENABLE_CATEGORY_OTHER OFF)
endif()

set (ZXING_ENABLE_1D_EFFECTIVE OFF)
if (ZXING_ENABLE_CATEGORY_ONED AND (ZXING_ENABLE_1D OR ZXING_ENABLE_CODABAR OR ZXING_ENABLE_CODE_39 OR ZXING_ENABLE_CODE_93
    OR ZXING_ENABLE_CODE_128 OR ZXING_ENABLE_DATA_BAR OR ZXING_ENABLE_DATA_BAR_EXPANDED OR ZXING_ENABLE_DATA_BAR_LIMITED
    OR ZXING_ENABLE_DX_FILM_EDGE OR ZXING_ENABLE_EAN_8 OR ZXING_ENABLE_EAN_13 OR ZXING_ENABLE_ITF OR ZXING_ENABLE_UPC_A
    OR ZXING_ENABLE_UPC_E))
    set (ZXING_ENABLE_1D_EFFECTIVE ON)
endif()
zxing_apply_enable(ZXING_ENABLE_AZTEC_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX ZXING_ENABLE_AZTEC)
zxing_apply_enable(ZXING_ENABLE_DATAMATRIX_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX ZXING_ENABLE_DATAMATRIX)
zxing_apply_enable(ZXING_ENABLE_MAXICODE_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX ZXING_ENABLE_MAXICODE)
set (ZXING_ENABLE_QRCODE_EFFECTIVE OFF)
if (ZXING_ENABLE_CATEGORY_MATRIX AND (ZXING_ENABLE_QRCODE OR ZXING_ENABLE_MICRO_QR_CODE OR ZXING_ENABLE_RMQR_CODE))
    set (ZXING_ENABLE_QRCODE_EFFECTIVE ON)
endif()
zxing_apply_enable(ZXING_ENABLE_MICRO_QR_CODE_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX ZXING_ENABLE_MICRO_QR_CODE)
zxing_apply_enable(ZXING_ENABLE_RMQR_CODE_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX ZXING_ENABLE_RMQR_CODE)
zxing_apply_enable(ZXING_ENABLE_PDF417_EFFECTIVE ZXING_ENABLE_CATEGORY_STACKED ZXING_ENABLE_PDF417)
zxing_apply_enable(ZXING_ENABLE_AUSTRALIA_POST_EFFECTIVE ZXING_ENABLE_CATEGORY_POSTAL ZXING_ENABLE_AUSTRALIA_POST)
zxing_apply_enable(ZXING_ENABLE_CODABAR_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_CODABAR)
zxing_apply_enable(ZXING_ENABLE_CODE_39_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_CODE_39)
zxing_apply_enable(ZXING_ENABLE_CODE_93_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_CODE_93)
zxing_apply_enable(ZXING_ENABLE_CODE_128_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_CODE_128)
zxing_apply_enable(ZXING_ENABLE_DATA_BAR_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_DATA_BAR)
zxing_apply_enable(ZXING_ENABLE_DATA_BAR_EXPANDED_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_DATA_BAR_EXPANDED)
zxing_apply_enable(ZXING_ENABLE_DATA_BAR_LIMITED_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_DATA_BAR_LIMITED)
zxing_apply_enable(ZXING_ENABLE_DX_FILM_EDGE_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_DX_FILM_EDGE)
zxing_apply_enable(ZXING_ENABLE_EAN_8_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_EAN_8)
zxing_apply_enable(ZXING_ENABLE_EAN_13_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_EAN_13)
zxing_apply_enable(ZXING_ENABLE_ITF_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_ITF)
zxing_apply_enable(ZXING_ENABLE_UPC_A_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_UPC_A)
zxing_apply_enable(ZXING_ENABLE_UPC_E_EFFECTIVE ZXING_ENABLE_CATEGORY_ONED ZXING_ENABLE_UPC_E)

if (BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (DEFINED BUILD_READERS)
    set (ZXING_READERS ${BUILD_READERS})
    message (WARNING "zxing-cpp cmake options BUILD_... are deprecated, please switch to ZXING_... variant")
endif()
if (DEFINED BUILD_WRITERS)
    set (ZXING_WRITERS ${BUILD_WRITERS})
    message (WARNING "zxing-cpp cmake options BUILD_... are deprecated, please switch to ZXING_... variant")
endif()

if (NOT DEFINED ZXING_WRITERS)
    set (ZXING_WRITERS OFF)
endif()

if (NOT DEFINED ZXING_READERS)
    set (ZXING_READERS ON)
endif()

set (ZXING_WRITERS_NEW OFF)
set (ZXING_WRITERS_OLD OFF)
if (ZXING_WRITERS MATCHES "OLD|ON")
    set (ZXING_WRITERS ON)
    set (ZXING_WRITERS_OLD ON)
elseif (ZXING_WRITERS MATCHES "NEW")
    set (ZXING_WRITERS ON)
    set (ZXING_WRITERS_NEW ON)
elseif (ZXING_WRITERS MATCHES "BOTH")
    set (ZXING_WRITERS ON)
    set (ZXING_WRITERS_NEW ON)
    set (ZXING_WRITERS_OLD ON)
endif()

if (BUILD_SHARED_LIBS)
    set (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set (ZXING_PUBLIC_FLAGS
    $<$<BOOL:${ZXING_EXPERIMENTAL_API}>:-DZXING_EXPERIMENTAL_API>
    $<$<BOOL:${ZXING_WRITERS_NEW}>:-DZXING_USE_ZINT>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_ONED}>:-DZXING_WITH_CATEGORY_ONED>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_STACKED}>:-DZXING_WITH_CATEGORY_STACKED>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_COMPOSITE}>:-DZXING_WITH_CATEGORY_COMPOSITE>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_TWOTRACK}>:-DZXING_WITH_CATEGORY_TWOTRACK>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_POSTAL}>:-DZXING_WITH_CATEGORY_POSTAL>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_MATRIX}>:-DZXING_WITH_CATEGORY_MATRIX>
    $<$<BOOL:${ZXING_ENABLE_CATEGORY_OTHER}>:-DZXING_WITH_CATEGORY_OTHER>
)
foreach(format 1D AZTEC DATAMATRIX MAXICODE PDF417 QRCODE MICRO_QR_CODE RMQR_CODE AUSTRALIA_POST
               CODABAR CODE_39 CODE_93 CODE_128 DATA_BAR DATA_BAR_EXPANDED DATA_BAR_LIMITED DX_FILM_EDGE
               EAN_8 EAN_13 ITF UPC_A UPC_E)
    list (APPEND ZXING_PUBLIC_FLAGS $<$<BOOL:${ZXING_ENABLE_${format}_EFFECTIVE}>:-DZXING_WITH_${format}>)
endforeach()

if (WINRT)
    set (ZXING_PUBLIC_FLAGS ${ZXING_PUBLIC_FLAGS}
        -DWINRT
    )
endif()
if (MSVC)
    set (ZXING_PUBLIC_FLAGS ${ZXING_PUBLIC_FLAGS}
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
    )
endif()

set (ZXING_PRIVATE_FLAGS
    $<$<BOOL:${ZXING_UNIT_TESTS}>:-DZXING_BUILD_FOR_TEST>
)
if (MSVC)
    set (ZXING_PRIVATE_FLAGS ${ZXING_PRIVATE_FLAGS}
        -D_SCL_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS
        -DNOMINMAX
        /utf-8 # see https://github.com/zxing-cpp/zxing-cpp/issues/757
    )
else()
    set (ZXING_PRIVATE_FLAGS ${ZXING_PRIVATE_FLAGS}
        -Wall -Wextra -Wno-missing-braces -Werror=undef -Werror=return-type)
endif()

include (CheckCXXCompilerFlag)

# This is needed for reproducible builds across different build directories.
# Without this, the usage of the __FILE__ macro leaves the build directory in
# the binary. When building the Python extension with build isolation enabled
# this would lead to random paths in the binary.
set(FILE_PREFIX_ARG "-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=")
check_cxx_compiler_flag("${FILE_PREFIX_ARG}" HAS_FILE_PREFIX_ARG)
if(HAS_FILE_PREFIX_ARG)
    set(ZXING_PRIVATE_FLAGS ${ZXING_PRIVATE_FLAGS} "${FILE_PREFIX_ARG}")
endif()

################# Source files

set (COMMON_FILES
    src/Barcode.h
    src/Barcode.cpp
    src/BarcodeFormat.h
    src/BarcodeFormat.cpp
    src/BitHacks.h
    src/BitMatrix.h
    src/BitMatrix.cpp
    src/BitMatrixIO.h
    src/BitMatrixIO.cpp
    src/ByteArray.h
    src/ByteMatrix.h
    src/CharacterSet.h
    src/CharacterSet.cpp
    src/Content.h
    src/Content.cpp
    src/DecoderResult.h
    src/DetectorResult.h
    src/ECI.h
    src/ECI.cpp
    src/Error.h
    src/Error.cpp
    src/Flags.h
    src/stacked/gs1/GTIN.h
    src/stacked/gs1/GTIN.cpp
    src/ImageView.h
    src/JSON.h
    src/JSON.cpp
    src/Matrix.h
    src/Point.h
    src/Quadrilateral.h
    src/Range.h
    src/reader/ReaderOptions.h
    src/reader/ReadBarcode.h
    src/reader/ReadBarcode.cpp
    src/Utf.h
    src/Utf.cpp
    src/reader/WriteBarcode.h
    src/reader/WriteBarcode.cpp
    src/ZXingCpp.h
    src/ZXingCpp.cpp
    src/ZXAlgorithms.h
    src/ZXConfig.h
    src/ZXTestSupport.h
    src/ZXVersion.h # [[deprecated]]
    $<$<BOOL:${ZXING_C_API}>:src/ZXingC.h>
    $<$<BOOL:${ZXING_C_API}>:src/ZXingC.cpp>
)
if (ZXING_READERS OR ZXING_WRITERS_OLD)
    set (COMMON_FILES ${COMMON_FILES}
        src/BitArray.h
        src/BitArray.cpp
        src/Generator.h
        src/GenericGF.h
        src/GenericGF.cpp
        src/GenericGFPoly.h
        src/GenericGFPoly.cpp
        src/TextUtfEncoding.h # [[deprecated]]
        src/TextUtfEncoding.cpp # [[deprecated]]
        src/Scope.h
    )
endif()
if (ZXING_READERS OR ZXING_WRITERS_NEW)
    set (COMMON_FILES ${COMMON_FILES}
        src/stacked/gs1/HRI.h
        src/stacked/gs1/HRI.cpp
    )
endif()
if (ZXING_READERS)
    set (COMMON_FILES ${COMMON_FILES}
        src/BinaryBitmap.h
        src/BinaryBitmap.cpp
        src/BitMatrixCursor.h
        src/BitSource.h
        src/BitSource.cpp
        src/ConcentricFinder.h
        src/ConcentricFinder.cpp
        src/reader/DecodeHints.h
        $<$<BOOL:${BUILD_SHARED_LIBS}>:src/reader/DecodeHints.cpp> # [[deprecated]]
        src/GlobalHistogramBinarizer.h
        src/GlobalHistogramBinarizer.cpp
        src/GridSampler.h
        src/GridSampler.cpp
        src/LogMatrix.h
        src/HybridBinarizer.h
        src/HybridBinarizer.cpp
        src/reader/MultiFormatReader.h
        src/reader/MultiFormatReader.cpp
        src/Pattern.h
        src/PerspectiveTransform.h
        src/PerspectiveTransform.cpp
        src/reader/Reader.h
        src/ReedSolomonDecoder.h
        src/ReedSolomonDecoder.cpp
        src/RegressionLine.h
        src/Result.h # [[deprecated]]
        src/ResultPoint.h
        src/ResultPoint.cpp
        src/StructuredAppend.h
        src/TextDecoder.h
        src/TextDecoder.cpp
        src/ThresholdBinarizer.h
        src/TritMatrix.h # QRCode
        src/WhiteRectDetector.h
        src/WhiteRectDetector.cpp
    )
endif()

if (ZXING_WRITERS)
    set (COMMON_FILES ${COMMON_FILES}
        src/TextEncoder.h
        src/TextEncoder.cpp
    )
endif()

if (ZXING_WRITERS_OLD)
    set (COMMON_FILES ${COMMON_FILES}
        src/ByteMatrix.h
        src/ReedSolomonEncoder.h
        src/ReedSolomonEncoder.cpp
        src/reader/MultiFormatWriter.h
        src/reader/MultiFormatWriter.cpp
    )
endif()

# define subset of public headers that get distributed with the binaries
set (PUBLIC_HEADERS
    src/Barcode.h
    src/BarcodeFormat.h
    src/BitHacks.h
    src/ByteArray.h
    src/CharacterSet.h
    src/Content.h
    src/Error.h
    src/Flags.h
    src/stacked/gs1/GTIN.h
    src/ImageView.h
    src/Point.h
    src/Quadrilateral.h
    src/Range.h # re-evaluate for 3.0
    src/reader/ReadBarcode.h
    src/reader/ReaderOptions.h
    src/StructuredAppend.h
    src/TextUtfEncoding.h # [[deprecated]]
    src/ZXingCpp.h
    src/ZXAlgorithms.h
    src/ZXVersion.h # [[deprecated]]
    $<$<BOOL:${ZXING_C_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/ZXingC.h>
    $<$<BOOL:${ZXING_EXPERIMENTAL_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/reader/WriteBarcode.h>
)
if (ZXING_READERS)
    set (PUBLIC_HEADERS ${PUBLIC_HEADERS}
        src/reader/DecodeHints.h # [[deprecated]]
        src/Result.h # [[deprecated]]
    )
endif()
if (ZXING_WRITERS_OLD)
    set (PUBLIC_HEADERS ${PUBLIC_HEADERS}
        src/BitMatrix.h
        src/BitMatrixIO.h
        src/Matrix.h
        src/reader/MultiFormatWriter.h
    )
endif()
# end of public header set

if (ZXING_READERS AND ZXING_ENABLE_AZTEC_EFFECTIVE)
    set (AZTEC_FILES ${AZTEC_FILES}
        src/matrix/aztec/AZDecoder.h
        src/matrix/aztec/AZDecoder.cpp
        src/matrix/aztec/AZDetector.h
        src/matrix/aztec/AZDetector.cpp
        src/matrix/aztec/AZDetectorResult.h
        src/matrix/aztec/AZReader.h
        src/matrix/aztec/AZReader.cpp
    )
endif()
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_AZTEC_EFFECTIVE)
    set (AZTEC_FILES ${AZTEC_FILES}
        src/matrix/aztec/AZEncodingState.h
        src/matrix/aztec/AZEncoder.h
        src/matrix/aztec/AZEncoder.cpp
        src/matrix/aztec/AZHighLevelEncoder.h
        src/matrix/aztec/AZHighLevelEncoder.cpp
        src/matrix/aztec/AZToken.h
        src/matrix/aztec/AZToken.cpp
        src/matrix/aztec/AZWriter.h
        src/matrix/aztec/AZWriter.cpp
    )
endif()


if ((ZXING_READERS OR ZXING_WRITERS_OLD) AND ZXING_ENABLE_DATAMATRIX_EFFECTIVE)
    set (DATAMATRIX_FILES
        src/matrix/datamatrix/DMBitLayout.h
        src/matrix/datamatrix/DMBitLayout.cpp
        src/matrix/datamatrix/DMVersion.h
        src/matrix/datamatrix/DMVersion.cpp
    )
endif()
if (ZXING_READERS AND ZXING_ENABLE_DATAMATRIX_EFFECTIVE)
    set (DATAMATRIX_FILES ${DATAMATRIX_FILES}
        src/matrix/datamatrix/DMDataBlock.h
        src/matrix/datamatrix/DMDataBlock.cpp
        src/matrix/datamatrix/DMDecoder.h
        src/matrix/datamatrix/DMDecoder.cpp
        src/matrix/datamatrix/DMDetector.h
        src/matrix/datamatrix/DMDetector.cpp
        src/matrix/datamatrix/DMReader.h
        src/matrix/datamatrix/DMReader.cpp
    )
endif()
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_DATAMATRIX_EFFECTIVE)
    set (DATAMATRIX_FILES ${DATAMATRIX_FILES}
        src/matrix/datamatrix/DMECEncoder.h
        src/matrix/datamatrix/DMECEncoder.cpp
        src/matrix/datamatrix/DMEncoderContext.h
        src/matrix/datamatrix/DMHighLevelEncoder.h
        src/matrix/datamatrix/DMHighLevelEncoder.cpp
        src/matrix/datamatrix/DMSymbolInfo.h
        src/matrix/datamatrix/DMSymbolInfo.cpp
        src/matrix/datamatrix/DMSymbolShape.h
        src/matrix/datamatrix/DMWriter.h
        src/matrix/datamatrix/DMWriter.cpp
    )
endif()


if (ZXING_READERS AND ZXING_ENABLE_MAXICODE_EFFECTIVE)
    set (MAXICODE_FILES ${MAXICODE_FILES}
        src/matrix/maxicode/MCBitMatrixParser.h
        src/matrix/maxicode/MCBitMatrixParser.cpp
        src/matrix/maxicode/MCDecoder.h
        src/matrix/maxicode/MCDecoder.cpp
        src/matrix/maxicode/MCReader.h
        src/matrix/maxicode/MCReader.cpp
    )
endif()


if ((ZXING_READERS OR ZXING_WRITERS_OLD) AND ZXING_ENABLE_1D_EFFECTIVE)
    set (ONED_FILES
        src/oned/ODUPCEANCommon.h
        src/oned/ODUPCEANCommon.cpp
        src/oned/ODCode93Patterns.h
        src/oned/ODCode128Patterns.h
        src/oned/ODCode128Patterns.cpp
    )
endif()
if (ZXING_READERS AND ZXING_ENABLE_1D_EFFECTIVE)
    set (ONED_FILES ${ONED_FILES}
        src/postal/ODAustraliaPostReader.h
        src/postal/ODAustraliaPostReader.cpp
        src/oned/ODCodabarReader.h
        src/oned/ODCodabarReader.cpp
        src/oned/ODCode39Reader.h
        src/oned/ODCode39Reader.cpp
        src/oned/ODCode93Reader.h
        src/oned/ODCode93Reader.cpp
        src/oned/ODCode128Reader.h
        src/oned/ODCode128Reader.cpp
        src/oned/ODDataBarCommon.h
        src/oned/ODDataBarCommon.cpp
        src/oned/ODDataBarReader.h
        src/oned/ODDataBarReader.cpp
        src/oned/ODDataBarExpandedBitDecoder.h
        src/oned/ODDataBarExpandedBitDecoder.cpp
        src/oned/ODDataBarExpandedReader.h
        src/oned/ODDataBarExpandedReader.cpp
        src/oned/ODDataBarLimitedReader.h
        src/oned/ODDataBarLimitedReader.cpp
        src/oned/ODDXFilmEdgeReader.h
        src/oned/ODDXFilmEdgeReader.cpp
        src/oned/ODITFReader.h
        src/oned/ODITFReader.cpp
        src/oned/ODMultiUPCEANReader.h
        src/oned/ODMultiUPCEANReader.cpp
        src/oned/ODReader.h
        src/oned/ODReader.cpp
        src/oned/ODRowReader.h
    )
endif()
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_1D_EFFECTIVE)
    set (ONED_FILES ${ONED_FILES}
        src/oned/ODCodabarWriter.h
        src/oned/ODCodabarWriter.cpp
        src/oned/ODCode39Writer.h
        src/oned/ODCode39Writer.cpp
        src/oned/ODCode93Writer.h
        src/oned/ODCode93Writer.cpp
        src/oned/ODCode128Writer.h
        src/oned/ODCode128Writer.cpp
        src/oned/ODEAN8Writer.h
        src/oned/ODEAN8Writer.cpp
        src/oned/ODEAN13Writer.h
        src/oned/ODEAN13Writer.cpp
        src/oned/ODITFWriter.h
        src/oned/ODITFWriter.cpp
        src/oned/ODUPCEWriter.h
        src/oned/ODUPCEWriter.cpp
        src/oned/ODUPCAWriter.h
        src/oned/ODUPCAWriter.cpp
        src/oned/ODWriterHelper.h
        src/oned/ODWriterHelper.cpp
    )
endif()


if ((ZXING_READERS OR ZXING_WRITERS_OLD) AND ZXING_ENABLE_PDF417_EFFECTIVE)
    set (PDF417_FILES
        src/stacked/pdf417/ZXBigInteger.h
        src/stacked/pdf417/ZXBigInteger.cpp
    )
endif()
if (ZXING_READERS AND ZXING_ENABLE_PDF417_EFFECTIVE)
    set (PDF417_FILES ${PDF417_FILES}
        src/stacked/pdf417/PDFBarcodeMetadata.h
        src/stacked/pdf417/PDFBarcodeValue.h
        src/stacked/pdf417/PDFBarcodeValue.cpp
        src/stacked/pdf417/PDFBoundingBox.h
        src/stacked/pdf417/PDFBoundingBox.cpp
        src/stacked/pdf417/PDFCodeword.h
        src/stacked/pdf417/PDFCodewordDecoder.h
        src/stacked/pdf417/PDFCodewordDecoder.cpp
        src/stacked/pdf417/PDFCustomData.h
        src/stacked/pdf417/PDFDecoder.h
        src/stacked/pdf417/PDFDecoder.cpp
        src/stacked/pdf417/PDFDetectionResult.h
        src/stacked/pdf417/PDFDetectionResult.cpp
        src/stacked/pdf417/PDFDetectionResultColumn.h
        src/stacked/pdf417/PDFDetectionResultColumn.cpp
        src/stacked/pdf417/PDFDetector.h
        src/stacked/pdf417/PDFDetector.cpp
        src/stacked/pdf417/PDFModulusGF.h
        src/stacked/pdf417/PDFModulusGF.cpp
        src/stacked/pdf417/PDFModulusPoly.h
        src/stacked/pdf417/PDFModulusPoly.cpp
        src/stacked/pdf417/PDFReader.h
        src/stacked/pdf417/PDFReader.cpp
        src/stacked/pdf417/PDFScanningDecoder.h
        src/stacked/pdf417/PDFScanningDecoder.cpp
        src/stacked/pdf417/ZXNullable.h
    )
endif()
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_PDF417_EFFECTIVE)
    set (PDF417_FILES ${PDF417_FILES}
        src/stacked/pdf417/PDFCompaction.h
        src/stacked/pdf417/PDFEncoder.h
        src/stacked/pdf417/PDFEncoder.cpp
        src/stacked/pdf417/PDFHighLevelEncoder.h
        src/stacked/pdf417/PDFHighLevelEncoder.cpp
        src/stacked/pdf417/PDFWriter.h
        src/stacked/pdf417/PDFWriter.cpp
    )
endif()


if ((ZXING_READERS OR ZXING_WRITERS_OLD) AND ZXING_ENABLE_QRCODE_EFFECTIVE)
    set (QRCODE_FILES
        src/matrix/qrcode/QRCodecMode.h
        src/matrix/qrcode/QRCodecMode.cpp
        src/matrix/qrcode/QRErrorCorrectionLevel.h
        src/matrix/qrcode/QRErrorCorrectionLevel.cpp
        src/matrix/qrcode/QRVersion.h
        src/matrix/qrcode/QRVersion.cpp
    )
endif()
if (ZXING_READERS AND ZXING_ENABLE_QRCODE_EFFECTIVE)
    set (QRCODE_FILES ${QRCODE_FILES}
        src/matrix/qrcode/QRBitMatrixParser.h
        src/matrix/qrcode/QRBitMatrixParser.cpp
        src/matrix/qrcode/QRDataBlock.h
        src/matrix/qrcode/QRDataBlock.cpp
        src/matrix/qrcode/QRDataMask.h
        src/matrix/qrcode/QRDecoder.h
        src/matrix/qrcode/QRDecoder.cpp
        src/matrix/qrcode/QRDetector.h
        src/matrix/qrcode/QRDetector.cpp
        src/matrix/qrcode/QRECB.h
        src/matrix/qrcode/QRFormatInformation.h
        src/matrix/qrcode/QRFormatInformation.cpp
        src/matrix/qrcode/QRReader.h
        src/matrix/qrcode/QRReader.cpp
    )
endif()
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_QRCODE_EFFECTIVE)
    set (QRCODE_FILES ${QRCODE_FILES}
        src/matrix/qrcode/QREncoder.h
        src/matrix/qrcode/QREncoder.cpp
        src/matrix/qrcode/QREncodeResult.h
        src/matrix/qrcode/QRMaskUtil.h
        src/matrix/qrcode/QRMaskUtil.cpp
        src/matrix/qrcode/QRMatrixUtil.h
        src/matrix/qrcode/QRMatrixUtil.cpp
        src/matrix/qrcode/QRWriter.h
        src/matrix/qrcode/QRWriter.cpp
    )
endif()

source_group (Sources FILES ${COMMON_FILES})
source_group (Sources\\aztec FILES ${AZTEC_FILES})
source_group (Sources\\datamatrix FILES ${DATAMATRIX_FILES})
source_group (Sources\\maxicode FILES ${MAXICODE_FILES})
source_group (Sources\\oned FILES ${ONED_FILES})
source_group (Sources\\pdf417 FILES ${PDF417_FILES})
source_group (Sources\\qrcode FILES ${QRCODE_FILES})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_library (ZXing
    ${COMMON_FILES}
    ${AZTEC_FILES}
    ${DATAMATRIX_FILES}
    ${MAXICODE_FILES}
    ${ONED_FILES}
    ${PDF417_FILES}
    ${QRCODE_FILES}
)

target_include_directories (ZXing
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>" "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    INTERFACE "$<INSTALL_INTERFACE:include>"
)

target_compile_options (ZXing
    PUBLIC ${ZXING_PUBLIC_FLAGS}
    PRIVATE ${ZXING_PRIVATE_FLAGS}
)

target_compile_features(ZXing PUBLIC cxx_std_17)

target_link_libraries (ZXing PRIVATE Threads::Threads)

if (ZXING_READERS OR ZXING_WRITERS_OLD)
    set (LIBZUECI_FILES
        src/libzueci/zueci.c
        src/libzueci/zueci.h
    )
    set_source_files_properties(${LIBZUECI_FILES} PROPERTIES
        COMPILE_FLAGS "$<$<NOT:$<BOOL:${ZXING_READERS}>>:-DZUECI_EMBED_NO_TO_UTF> $<$<NOT:$<BOOL:${ZXING_WRITERS_OLD}>>:-DZUECI_EMBED_NO_TO_ECI>"
        SKIP_PRECOMPILE_HEADERS ON
    )
    target_sources(ZXing PRIVATE ${LIBZUECI_FILES})
    source_group (Sources\\libzueci FILES ${LIBZUECI_FILES})
endif()

if (ZXING_WRITERS_NEW)
    if (ZXING_USE_BUNDLED_ZINT)
        aux_source_directory(src/libzint LIBZINT_FILES) # manually re-run cmake after adding a new file/symlink
        set_source_files_properties(${LIBZINT_FILES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
        target_sources(ZXing PRIVATE ${LIBZINT_FILES})
        source_group (Sources\\libzint FILES ${LIBZINT_FILES})
        target_include_directories (ZXing PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libzint>")
    else()
        include(../zxing.cmake)
        zxing_add_package(zint zint https://github.com/zint/zint.git 2.16.0)
        target_link_libraries (ZXing PRIVATE zint)
    endif()
endif()

add_library(ZXing::ZXing ALIAS ZXing)
# add the old alias as well, to keep old clients compiling [[deprecated]]
# note: this only affects client code that includes ZXing via sub_directory.
#       for clients using the exported target, see ZXingConfig.cmake.in
add_library(ZXing::Core ALIAS ZXing)

set_target_properties(ZXing PROPERTIES EXPORT_NAME ZXing)
# force position independent code to be able to link it as static lib into a DLL (e.g. the python module)
set_target_properties(ZXing PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (PROJECT_VERSION)
    set_target_properties(ZXing PROPERTIES VERSION ${PROJECT_VERSION})
    set_target_properties(ZXing PROPERTIES SOVERSION ${ZXING_SONAME})
endif()

set_target_properties(ZXing PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

set(PRECOMPILE_HEADERS ${PUBLIC_HEADERS})
list(REMOVE_ITEM PRECOMPILE_HEADERS "$<$<BOOL:${ZXING_C_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/ZXingC.h>")
list(REMOVE_ITEM PRECOMPILE_HEADERS src/reader/DecodeHints.h) # [[deprecated]]
list(REMOVE_ITEM PRECOMPILE_HEADERS src/Result.h)      # [[deprecated]]
list(REMOVE_ITEM PRECOMPILE_HEADERS src/ZXVersion.h)   # [[deprecated]]
target_precompile_headers(ZXing PRIVATE ${PRECOMPILE_HEADERS})
set_source_files_properties(src/reader/DecodeHints.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
    # The following is a list of translation units that fulfill two criteria regarding the use of -Os vs -O3:
    #  1. their binary size decreases significantly
    #  2. the runtime of ReaderTest is not (measurably) affected
    # Compiling them with -Os saves about 40kB (3%) with clang and 190kB (12%) with gcc.
    check_cxx_compiler_flag("-Os" COMPILER_KNOWS_Os)
    if(COMPILER_KNOWS_Os)
    set_source_files_properties(
        src/Barcode.cpp
        src/BarcodeFormat.cpp
        src/BitMatrixIO.cpp
        src/Error.cpp
        src/stacked/gs1/GTIN.cpp
        src/stacked/gs1/HRI.cpp
        src/reader/MultiFormatReader.cpp
        src/reader/WriteBarcode.cpp
        src/ZXingC.cpp
        src/ZXingCpp.cpp
        src/matrix/aztec/AZHighLevelEncoder.cpp
        src/matrix/datamatrix/DMDataBlock.cpp
        src/matrix/datamatrix/DMHighLevelEncoder.cpp
        src/oned/ODDataBarExpandedBitDecoder.cpp
        src/stacked/pdf417/PDFHighLevelEncoder.cpp
        src/matrix/qrcode/QRBitMatrixParser.cpp
        src/matrix/qrcode/QRDataBlock.cpp
        src/matrix/qrcode/QRDecoder.cpp
        src/matrix/qrcode/QREncoder.cpp
        src/matrix/qrcode/QRMaskUtil.cpp
        src/matrix/qrcode/QRReader.cpp
        src/matrix/qrcode/QRVersion.cpp
        ${LIBZINT_FILES}
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON COMPILE_FLAGS -Os)
    endif()
endif()

include (GNUInstallDirs)

set(ZX_INSTALL_TARGETS ZXing)

install (
    TARGETS ${ZX_INSTALL_TARGETS} EXPORT ZXingTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ZXing"
)

configure_file (Version.h.in Version.h)

install (
    FILES "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ZXing"
)

if (MSVC)
    set_target_properties(ZXing PROPERTIES
        COMPILE_PDB_NAME ZXing
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ZXing.pdb
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            CONFIGURATIONS Debug RelWithDebInfo
            OPTIONAL)
endif()

set (CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ZXing")

install (
    EXPORT ZXingTargets
    DESTINATION ${CMAKECONFIG_INSTALL_DIR} NAMESPACE ZXing::
)

IF (NOT WIN32 OR MINGW)
    if (${ZXING_EXPERIMENTAL_API})
        set(COMPILE_FLAGS "-DZXING_EXPERIMENTAL_API")
    endif()
    configure_file(zxing.pc.in zxing.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zxing.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
ENDIF()

include (CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ZXingConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file (
    ZXingConfig.cmake.in
    ZXingConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install (
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZXingConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZXingConfigVersion.cmake"
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)
