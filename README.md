[![Build Status](https://github.com/zxing-cpp/zxing-cpp/workflows/CI/badge.svg?branch=master)](https://github.com/zxing-cpp/zxing-cpp/actions?query=workflow%3ACI)

# ZXing-C++

ZXing-C++ ("zebra crossing") is an open-source, multi-format linear/matrix barcode image processing library implemented in C++.

It was originally ported from the Java [ZXing Library](https://github.com/zxing/zxing) but has been developed further and now includes many improvements in terms of runtime and detection performance. It can both read and write barcodes in a number of formats.

## Sponsors

You can sponsor this library at [GitHub Sponsors](https://github.com/sponsors/axxel).

Named Sponsors:
* [KURZ Digital Solutions GmbH & Co. KG](https://github.com/kurzdigital)
* [Useful Sensors Inc](https://github.com/usefulsensors)
* [synedra](https://synedra.com/)

Thanks a lot for your contribution!

## Features

* Written in pure C++20 (/C++17), no third-party dependencies (for the library itself)
* Thread safe
* Wrappers/Bindings for:
  * [Android](wrappers/android/README.md)
  * [C](wrappers/c/README.md)
  * [iOS](wrappers/ios/README.md)
  * [Kotlin/Native](wrappers/kn/README.md)
  * [.NET](wrappers/dotnet/README.md)
  * [Python](wrappers/python/README.md)
  * [Rust](wrappers/rust/README.md)
  * [WebAssembly](wrappers/wasm/README.md)
  * [WinRT](wrappers/winrt/README.md)
  * [Flutter](https://pub.dev/packages/flutter_zxing) (external project)

## Supported Formats

The library supports reading **all 56 barcode formats** listed below:

### 1D Product Codes
| Format | Description |
|--------|-------------|
| UPC-A | Universal Product Code (12 digits) |
| UPC-E | UPC-E (6-digit compressed format) |
| EAN-8 | European Article Number (8 digits) |
| EAN-13 | European Article Number (13 digits) |
| DataBar | GS1 DataBar (formerly RSS-14) |
| DataBar Expanded | GS1 DataBar Expanded |
| DataBar Limited | GS1 DataBar Limited |
| DataBar Stacked | GS1 DataBar Stacked (2-row variant) |
| DataBar Stacked Omnidirectional | GS1 DataBar Stacked Omni |
| DataBar Expanded Stacked | GS1 DataBar Expanded Stacked |

### 1D Industrial Codes
| Format | Description |
|--------|-------------|
| Code 39 | Code 39 (including Extended variant) |
| Code 93 | Code 93 |
| Code 128 | Code 128 (with automatic subset switching) |
| Codabar | Codabar |
| ITF | Interleaved 2 of 5 (ITF-14) |
| Code 11 | Code 11 (USD-8, telecommunications) |
| MSI | MSI (Modified Plessey) |
| Telepen | Telepen (Full ASCII) |
| LOGMARS | LOGMARS (Code 39 variant, US military) |
| DX Film Edge | DX Film Edge Barcode |
| Channel Code | ANSI/AIM BC12 (compact numeric) |

### Postal Codes (4-State)
| Format | Description |
|--------|-------------|
| Australia Post | Australia Post 4-State (Standard, Reply Paid, Routing, Redirection) |
| KIX Code | Dutch Post KIX Code 4-State |
| Japan Post | Japan Post 4-State (Kasutama Barcode) |
| Korea Post | Korea Post Barcode |
| RM4SCC | Royal Mail 4-State Customer Code (UK) |
| Mailmark | Royal Mail 4-State Mailmark (UK, Types C & L) |
| USPS IMB | USPS Intelligent Mail Barcode (OneCode, 4CB) |
| POSTNET | USPS POSTNET |
| PLANET | USPS PLANET |
| Deutsche Post Leitcode | Deutsche Post Leitcode (14 digits, German routing) |
| Deutsche Post Identcode | Deutsche Post Identcode (12 digits, German ID) |

### Pharmaceutical Codes
| Format | Description |
|--------|-------------|
| Code 32 | Code 32 (Italian Pharmacode) |
| Pharmacode | Pharmacode (Laetus, binary) |
| Pharmacode Two-Track | Pharmacode Two-Track (Laetus, 3-state) |
| PZN | Pharmazentralnummer (German pharmaceutical) |

### 2-of-5 Family
| Format | Description |
|--------|-------------|
| Matrix 2 of 5 | Matrix 2 of 5 (Standard 2 of 5) |
| Industrial 2 of 5 | Industrial 2 of 5 (bars-only) |
| IATA 2 of 5 | IATA 2 of 5 (Airline, air cargo) |
| Datalogic 2 of 5 | Datalogic 2 of 5 (China Post) |

### Stacked Linear Codes
| Format | Description |
|--------|-------------|
| Codablock-F | Codablock F (stacked Code 128, 2-44 rows) |
| Code 16K | Code 16K (stacked Code 128, 2-16 rows) |
| Code 49 | Code 49 (USS-49, stacked 2-8 rows) |

### 2D Matrix Codes
| Format | Description |
|--------|-------------|
| QR Code | QR Code |
| Micro QR Code | Micro QR Code |
| rMQR Code | Rectangular Micro QR Code |
| Aztec | Aztec |
| Aztec Rune | Aztec Rune (compact 11x11, encodes 0-255) |
| DataMatrix | DataMatrix (ECC200) |
| PDF417 | PDF417 (including Truncated variant) |
| MaxiCode | MaxiCode (partial support) |
| Code One | Code One (2D matrix, versions A-H/S/T) |
| DotCode | DotCode (2D dot matrix, high-speed printing) |
| Grid Matrix | Grid Matrix (Chinese standard GB/T 21049) |
| Han Xin Code | Han Xin Code (Chinese standard, ISO/IEC 20830) |
| UPN QR | UPN QR Code (Slovenian payment QR) |

### Notes

**Reading:**
* All 56 formats listed above are supported for reading.
* MaxiCode has partial support (requires unrotated, unskewed images).
* Building with C++17 instead of C++20 disables DataBar Limited reading and reduces DataMatrix multi-symbol detection.

**Writing:**
* Standard writers (default): Aztec, Codabar, Code 39, Code 93, Code 128, DataMatrix, EAN-8, EAN-13, ITF, PDF417, QR Code, UPC-A, UPC-E.
* Extended writers (with `ZXING_WRITERS=NEW` and `ZXING_EXPERIMENTAL_API=ON`): All formats except DataBar variants, DX Film Edge, MaxiCode, Micro QR Code, and rMQR Code.

## Getting Started

### To read barcodes:
1. Load your image into memory (3rd-party library required).
2. Call `ReadBarcodes()` from [`ReadBarcode.h`](core/src/ReadBarcode.h), the simplest API to get a list of `Barcode` objects.

A very simple example looks like this:
```c++
#include "ZXing/ReadBarcode.h"
#include <iostream>

int main(int argc, char** argv)
{
    int width, height;
    unsigned char* data;
    // load your image data from somewhere. ImageFormat::Lum assumes grey scale image data.

    auto image = ZXing::ImageView(data, width, height, ZXing::ImageFormat::Lum);
    auto options = ZXing::ReaderOptions().setFormats(ZXing::BarcodeFormat::Any);
    auto barcodes = ZXing::ReadBarcodes(image, options);

    for (const auto& b : barcodes)
        std::cout << ZXing::ToString(b.format()) << ": " << b.text() << "\n";

    return 0;
}
```
To see the full capability of the API, have a look at [`ZXingReader.cpp`](example/ZXingReader.cpp).

[Note: At least C++17 is required on the client side to use the API.]

### To write barcodes:
1. Create a [`MultiFormatWriter`](core/src/MultiFormatWriter.h) instance with the format you want to generate. Set encoding and margins if needed.
2. Call `encode()` with text content and the image size. This returns a [`BitMatrix`](core/src/BitMatrix.h) which is a binary image of the barcode where `true` == visual black and `false` == visual white.
3. Convert the bit matrix to your native image format. See also the `ToMatrix<T>(BitMatrix&)` helper function.

As an example, have a look at [`ZXingWriter.cpp`](example/ZXingWriter.cpp). That file also contains example code showing the new `ZXING_EXPERIMENTAL_API` for writing barcodes.

## Web Demos
- [Read barcodes](https://zxing-cpp.github.io/zxing-cpp/demo_reader.html)
- [Write barcodes](https://zxing-cpp.github.io/zxing-cpp/demo_writer.html)
- [Read barcodes from camera](https://zxing-cpp.github.io/zxing-cpp/demo_cam_reader.html)

[Note: those live demos are not necessarily fully up-to-date at all times.]

## Build Instructions
These are the generic instructions to build the library on Windows/macOS/Linux. For details on how to build the individual wrappers, follow the links above.

1. Make sure [CMake](https://cmake.org) version 3.16 or newer is installed. The python module requires 3.18 or higher.
2. Make sure a sufficiently C++20 compliant compiler is installed (minimum VS 2019 16.10? / gcc 11 / clang 12?).
3. See the cmake `ZXING_...` options to enable the testing code, python wrapper, etc.

```
git clone https://github.com/zxing-cpp/zxing-cpp.git --recursive --single-branch --depth 1
cmake -S zxing-cpp -B zxing-cpp.release -DCMAKE_BUILD_TYPE=Release
cmake --build zxing-cpp.release -j8 --config Release
```

[Note: binary packages are available for/as
[vcpkg](https://github.com/Microsoft/vcpkg/tree/master/ports/nu-book-zxing-cpp),
[conan](https://github.com/conan-io/conan-center-index/tree/master/recipes/zxing-cpp),
[mingw](https://github.com/msys2/MINGW-packages/tree/master/mingw-w64-zxing-cpp) and a bunch of
[linux distributions](https://repology.org/project/zxing-cpp/versions).]
