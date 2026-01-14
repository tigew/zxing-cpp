zxing_add_package(GTest googletest https://github.com/google/googletest.git v1.15.2)

if (GTest_POPULATED)
    # don't install gtest stuff on "make install"
    set (INSTALL_GTEST OFF CACHE BOOL "" FORCE)
endif()

if (MSVC)
    # default to UTF-8
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

add_executable (UnitTest
    BarcodeFormatTest.cpp
    BitArrayUtility.cpp
    BitArrayUtility.h
    BitHacksTest.cpp
    CharacterSetECITest.cpp
    ContentTest.cpp
    ErrorTest.cpp
    GTINTest.cpp
    JSONTest.cpp
    PseudoRandom.h
    SanitizerSupport.cpp
    TextUtfEncodingTest.cpp
    ZXAlgorithmsTest.cpp
)

if (ZXING_READERS)
target_sources (UnitTest PRIVATE
    PatternTest.cpp
    TextDecoderTest.cpp
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:ThresholdBinarizerTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_AZTEC_EFFECTIVE}>:matrix/aztec/AZDecoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_AZTEC_EFFECTIVE}>:matrix/aztec/AZDetectorTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_DATAMATRIX_EFFECTIVE}>:matrix/datamatrix/DMDecodedBitStreamParserTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_MAXICODE_EFFECTIVE}>:matrix/maxicode/MCDecoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode128ReaderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode39ExtendedModeTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode39ReaderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode93ReaderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODDataBarExpandedBitDecoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODDataBarReaderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_PDF417_EFFECTIVE}>:stacked/pdf417/PDF417DecoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_PDF417_EFFECTIVE}>:stacked/pdf417/PDF417ErrorCorrectionTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_PDF417_EFFECTIVE}>:stacked/pdf417/PDF417ScanningDecoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/MQRDecoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRBitMatrixParserTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRDataMaskTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRDecodedBitStreamParserTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRErrorCorrectionLevelTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRFormatInformationTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRModeTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRVersionTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/RMQRDecoderTest.cpp>
)
endif()

if (ZXING_WRITERS MATCHES "ON|OLD|BOTH")
target_sources (UnitTest PRIVATE
    $<$<BOOL:${ZXING_ENABLE_AZTEC_EFFECTIVE}>:matrix/aztec/AZEncoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_DATAMATRIX_EFFECTIVE}>:matrix/datamatrix/DMHighLevelEncodeTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_DATAMATRIX_EFFECTIVE}>:matrix/datamatrix/DMPlacementTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_DATAMATRIX_EFFECTIVE}>:matrix/datamatrix/DMSymbolInfoTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_DATAMATRIX_EFFECTIVE}>:matrix/datamatrix/DMWriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode39WriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode93WriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODEAN13WriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODEAN8WriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODITFWriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODUPCAWriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODUPCEWriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_PDF417_EFFECTIVE}>:stacked/pdf417/PDF417HighLevelEncoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_PDF417_EFFECTIVE}>:stacked/pdf417/PDF417WriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QRWriterTest.cpp>
)
endif()

if (ZXING_READERS AND ZXING_WRITERS MATCHES "ON|OLD|BOTH")
target_sources (UnitTest PRIVATE
    ReedSolomonTest.cpp
    $<$<BOOL:${ZXING_ENABLE_AZTEC_EFFECTIVE}>:matrix/aztec/AZEncodeDecodeTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_AZTEC_EFFECTIVE}>:matrix/aztec/AZHighLevelEncoderTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_DATAMATRIX_EFFECTIVE}>:matrix/datamatrix/DMEncodeDecodeTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCodaBarWriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_1D_EFFECTIVE}>:oned/ODCode128WriterTest.cpp>
    $<$<BOOL:${ZXING_ENABLE_QRCODE_EFFECTIVE}>:matrix/qrcode/QREncoderTest.cpp>
)
endif()

if (ZXING_READERS OR ZXING_WRITERS MATCHES "NEW|BOTH")
target_sources (UnitTest PRIVATE
    GS1Test.cpp
)
endif()

if (ZXING_WRITERS MATCHES "NEW|BOTH")
target_sources (UnitTest PRIVATE
    WriteBarcodeTest.cpp
    TextEncoderTest.cpp
)
endif()

target_include_directories (UnitTest PRIVATE .)

target_link_libraries (UnitTest ZXing::ZXing GTest::gtest_main GTest::gmock)

#target_precompile_headers(UnitTest PRIVATE ${CMAKE_SOURCE_DIR}/core/src/reader/ReadBarcode.h)

add_test(NAME UnitTest COMMAND UnitTest)
