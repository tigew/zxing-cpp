// swift-tools-version:5.7.1
import PackageDescription

let package = Package(
    name: "ZXingCpp",
    platforms: [
        .iOS(.v11)
    ],
    products: [
        .library(
            name: "ZXingCpp",
            targets: ["ZXingCpp"])
    ],
    targets: [
        .target(
            name: "ZXingCppCore",
            path: "core/src",
            exclude: ["libzint", "ZXingC.cpp", "ZXingCpp.cpp"],
            publicHeadersPath: ".",
            cxxSettings: [
                // Master reader flag
                .define("ZXING_READERS"),

                // === 1D / Linear Barcode Readers ===
                .define("ZXING_WITH_1D"),

                // === 2D / Matrix Barcode Readers ===
                .define("ZXING_WITH_QRCODE"),
                .define("ZXING_WITH_AZTEC"),
                .define("ZXING_WITH_AZTECRUNE"),
                .define("ZXING_WITH_CODEONE"),
                .define("ZXING_WITH_DATAMATRIX"),
                .define("ZXING_WITH_DOTCODE"),
                .define("ZXING_WITH_GRIDMATRIX"),
                .define("ZXING_WITH_HANXIN"),
                .define("ZXING_WITH_MAXICODE"),
                .define("ZXING_WITH_PDF417"),
                .define("ZXING_WITH_MICROQRCODE"),
                .define("ZXING_WITH_RMQRCODE"),
                .define("ZXING_WITH_UPNQR"),

                // === Retail / UPC / EAN ===
                .define("ZXING_WITH_EAN8"),
                .define("ZXING_WITH_EAN13"),
                .define("ZXING_WITH_UPCA"),
                .define("ZXING_WITH_UPCE"),

                // === Linear Codes ===
                .define("ZXING_WITH_CODE39"),
                .define("ZXING_WITH_CODE93"),
                .define("ZXING_WITH_CODE128"),
                .define("ZXING_WITH_CODABAR"),
                .define("ZXING_WITH_ITF"),
                .define("ZXING_WITH_CODE11"),
                .define("ZXING_WITH_MSI"),
                .define("ZXING_WITH_TELEPEN"),
                .define("ZXING_WITH_LOGMARS"),
                .define("ZXING_WITH_DXFILMEDGE"),
                .define("ZXING_WITH_CHANNELCODE"),

                // === DataBar Variants ===
                .define("ZXING_WITH_DATABAR"),
                .define("ZXING_WITH_DATABAREXPANDED"),
                .define("ZXING_WITH_DATABARLIMITED"),
                .define("ZXING_WITH_DATABARSTACKED"),
                .define("ZXING_WITH_DATABARSTACKEDOMNIDIRECTIONAL"),
                .define("ZXING_WITH_DATABAREXPANDEDSTACKED"),

                // === Postal Codes ===
                .define("ZXING_WITH_AUSTRALIAPOST"),
                .define("ZXING_WITH_KIXCODE"),
                .define("ZXING_WITH_JAPANPOST"),
                .define("ZXING_WITH_KOREAPOST"),
                .define("ZXING_WITH_RM4SCC"),
                .define("ZXING_WITH_MAILMARK"),
                .define("ZXING_WITH_USPSIMB"),
                .define("ZXING_WITH_DEUTSCHEPOSTLEITCODE"),
                .define("ZXING_WITH_DEUTSCHEPOSTIDENTCODE"),
                .define("ZXING_WITH_POSTNET"),
                .define("ZXING_WITH_PLANET"),

                // === Pharmaceutical / Industrial ===
                .define("ZXING_WITH_CODE32"),
                .define("ZXING_WITH_PHARMACODE"),
                .define("ZXING_WITH_PHARMACODETWOTRACK"),
                .define("ZXING_WITH_PZN"),
                .define("ZXING_WITH_MATRIX2OF5"),
                .define("ZXING_WITH_INDUSTRIAL2OF5"),
                .define("ZXING_WITH_IATA2OF5"),
                .define("ZXING_WITH_DATALOGIC2OF5"),
                .define("ZXING_WITH_CODABLOCKF"),
                .define("ZXING_WITH_CODE16K"),
                .define("ZXING_WITH_CODE49"),

                // === ENABLE flags (runtime format support) ===
                // Retail / UPC / EAN
                .define("ZXING_ENABLE_EAN8"),
                .define("ZXING_ENABLE_EAN13"),
                .define("ZXING_ENABLE_UPCA"),
                .define("ZXING_ENABLE_UPCE"),

                // Linear Codes
                .define("ZXING_ENABLE_CODE39"),
                .define("ZXING_ENABLE_CODE93"),
                .define("ZXING_ENABLE_CODE128"),
                .define("ZXING_ENABLE_CODABAR"),
                .define("ZXING_ENABLE_ITF"),
                .define("ZXING_ENABLE_CODE11"),
                .define("ZXING_ENABLE_MSI"),
                .define("ZXING_ENABLE_TELEPEN"),
                .define("ZXING_ENABLE_LOGMARS"),
                .define("ZXING_ENABLE_DXFILMEDGE"),
                .define("ZXING_ENABLE_CHANNELCODE"),

                // DataBar Variants
                .define("ZXING_ENABLE_DATABAR"),
                .define("ZXING_ENABLE_DATABAREXPANDED"),
                .define("ZXING_ENABLE_DATABARLIMITED"),
                .define("ZXING_ENABLE_DATABARSTACKED"),
                .define("ZXING_ENABLE_DATABARSTACKEDOMNIDIRECTIONAL"),
                .define("ZXING_ENABLE_DATABAREXPANDEDSTACKED"),

                // Postal Codes
                .define("ZXING_ENABLE_AUSTRALIAPOST"),
                .define("ZXING_ENABLE_KIXCODE"),
                .define("ZXING_ENABLE_JAPANPOST"),
                .define("ZXING_ENABLE_KOREAPOST"),
                .define("ZXING_ENABLE_RM4SCC"),
                .define("ZXING_ENABLE_MAILMARK"),
                .define("ZXING_ENABLE_USPSIMB"),
                .define("ZXING_ENABLE_DEUTSCHEPOSTLEITCODE"),
                .define("ZXING_ENABLE_DEUTSCHEPOSTIDENTCODE"),
                .define("ZXING_ENABLE_POSTNET"),
                .define("ZXING_ENABLE_PLANET"),

                // Pharmaceutical / Industrial
                .define("ZXING_ENABLE_CODE32"),
                .define("ZXING_ENABLE_PHARMACODE"),
                .define("ZXING_ENABLE_PHARMACODETWOTRACK"),
                .define("ZXING_ENABLE_PZN"),
                .define("ZXING_ENABLE_MATRIX2OF5"),
                .define("ZXING_ENABLE_INDUSTRIAL2OF5"),
                .define("ZXING_ENABLE_IATA2OF5"),
                .define("ZXING_ENABLE_DATALOGIC2OF5"),
                .define("ZXING_ENABLE_CODABLOCKF"),
                .define("ZXING_ENABLE_CODE16K"),
                .define("ZXING_ENABLE_CODE49"),

                // 2D / Matrix Codes
                .define("ZXING_ENABLE_QRCODE"),
                .define("ZXING_ENABLE_MICROQRCODE"),
                .define("ZXING_ENABLE_RMQRCODE"),
                .define("ZXING_ENABLE_UPNQR"),
                .define("ZXING_ENABLE_AZTEC"),
                .define("ZXING_ENABLE_AZTECRUNE"),
                .define("ZXING_ENABLE_CODEONE"),
                .define("ZXING_ENABLE_DATAMATRIX"),
                .define("ZXING_ENABLE_DOTCODE"),
                .define("ZXING_ENABLE_GRIDMATRIX"),
                .define("ZXING_ENABLE_HANXIN"),
                .define("ZXING_ENABLE_MAXICODE"),
                .define("ZXING_ENABLE_PDF417")
            ]
        ),
        .target(
            name: "ZXingCpp",
            dependencies: ["ZXingCppCore"],
            path: "wrappers/ios/Sources/Wrapper",
            publicHeadersPath: ".",
            linkerSettings: [
                .linkedFramework("CoreGraphics"),
                .linkedFramework("CoreImage"),
                .linkedFramework("CoreVideo")
            ]
        )
    ],
    cxxLanguageStandard: CXXLanguageStandard.gnucxx20
)
