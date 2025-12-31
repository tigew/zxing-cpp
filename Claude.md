# ZXing-C++ Barcode Format Implementation Guide

This document tracks the implementation status of barcode formats in zxing-cpp and provides guidelines for adding new format support.

## Table of Contents
- [Implementation Policy](#implementation-policy)
- [Current Implementation Status](#current-implementation-status)
- [Missing Formats to Implement](#missing-formats-to-implement)
- [Coding Conventions](#coding-conventions)
- [Implementation Patterns](#implementation-patterns)
- [Wrapper Updates Required](#wrapper-updates-required)
- [Implementation Roadmap](#implementation-roadmap)

---

## Implementation Policy

**IMPORTANT: All implementations MUST be fully functional. No skeleton, stub, or placeholder implementations are acceptable.**

When implementing a new barcode format reader or writer:

1. **Complete Functionality Required**: Every reader must fully decode the barcode format, including:
   - Pattern detection and recognition
   - Data decoding with proper character set handling
   - Error correction where applicable (e.g., Reed-Solomon)
   - All format variants must be supported

2. **No Half-Baked Implementations**: Do not commit code that:
   - Returns empty results without attempting decoding
   - Has TODO comments for critical functionality
   - Only handles a subset of the format's variants
   - Lacks proper error detection/correction

3. **Test Before Commit**: Ensure the implementation can decode real-world barcode samples before marking as complete.

4. **Update All Layers**: Each implementation must include updates to:
   - Core library (BarcodeFormat enum, reader/writer classes)
   - CMake build configuration
   - iOS wrapper (ZXIFormat.h, ZXIFormatHelper.mm)
   - C API (ZXingC.h)
   - This documentation

5. **Format Migration**: When a format is fully implemented:
   - Remove it from the "Missing Formats to Implement" section
   - Add it to the "Fully Supported Formats" table
   - Update the "Progress Tracking" section with implementation details

---

## Current Implementation Status

### Fully Supported Formats (32 formats)

| Format | Read | Write (OLD) | Write (NEW/Zint) | Notes |
|--------|------|-------------|------------------|-------|
| AustraliaPost | Yes | No | Yes | 4-state postal, 6 FCC variants (11, 45, 59, 62, 87, 92) |
| Aztec | Yes | Yes | Yes | Full support |
| Codabar | Yes | Yes | Yes | |
| Code11 | Yes | No | Yes | Telecommunications (USD-8), check digit C and K |
| POSTNET | Yes | No | Yes | USPS POSTNET (height-modulated 5-bar encoding) |
| PLANET | Yes | No | Yes | USPS PLANET (inverse of POSTNET encoding) |
| Code39 | Yes | Yes | Yes | Includes Extended variant |
| Code93 | Yes | Yes | Yes | |
| Code128 | Yes | Yes | Yes | Automatic subset switching |
| DataBar | Yes | No | Yes | GS1 DataBar (formerly RSS-14) |
| DataBarExpanded | Yes | No | Yes | GS1 DataBar Expanded |
| DataBarLimited | Yes | No | Yes | Requires C++20 for reading |
| DataMatrix | Yes | Yes | Yes | ECC200 |
| DXFilmEdge | Yes | No | Yes | |
| EAN-8 | Yes | Yes | Yes | |
| EAN-13 | Yes | Yes | Yes | |
| ITF | Yes | Yes | Yes | Interleaved 2 of 5, ITF-14 |
| JapanPost | Yes | No | Yes | Japan Post 4-State (Kasutama), 7-digit postal + address |
| KIXCode | Yes | No | Yes | Dutch Post 4-state (Netherlands), same encoding as RM4SCC |
| KoreaPost | Yes | No | Yes | Korea Post (Korean Postal Authority), 6-digit postal + check |
| Mailmark | Yes | No | Yes | Royal Mail 4-State Mailmark (UK postal), Types C (66 bars) and L (78 bars), RS error correction |
| RM4SCC | Yes | No | Yes | Royal Mail 4-State Customer Code (UK postal), checksum validated |
| USPSIMB | Yes | No | Yes | USPS Intelligent Mail Barcode (OneCode, 4CB), 65-bar 4-state, CRC-11 |
| DeutschePostLeitcode | Yes | No | Yes | Deutsche Post Leitcode (14 digits, German routing), ITF-based |
| DeutschePostIdentcode | Yes | No | Yes | Deutsche Post Identcode (12 digits, German ID), ITF-based |
| MaxiCode | Yes (partial) | No | Yes | |
| MicroQRCode | Yes | No | Yes | |
| PDF417 | Yes | Yes | Yes | Includes Truncated variant |
| QRCode | Yes | Yes | Yes | |
| RMQRCode | Yes | No | Yes | Rectangular Micro QR Code |
| UPC-A | Yes | Yes | Yes | |
| UPC-E | Yes | Yes | Yes | |

---

## Missing Formats to Implement

### Priority 1: Industrial/Linear Codes

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| MSI (Modified Plessey) | Linear | Low | Inventory/warehousing |
| Telepen | Linear | Low | UK library system |
| LOGMARS | Linear | Low | Code 39 variant for military |
| Code 32 | Linear | Low | Italian Pharmacode (Code 39 variant) |
| Pharmacode | Linear | Low | Pharmaceutical |
| Pharmacode Two-Track | Linear | Medium | Pharmaceutical |
| Pharmazentralnummer | Linear | Low | German pharmaceutical (Code 39 variant) |
| Channel Code | Linear | Medium | Compact numeric encoding |

### Priority 2: Code 2 of 5 Variants

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Matrix 2 of 5 | Linear | Low | Also known as Code 25 |
| Industrial 2 of 5 | Linear | Low | |
| IATA 2 of 5 | Linear | Low | Airline industry |
| Datalogic 2 of 5 | Linear | Low | |

### Priority 3: Stacked/Multi-Row Linear

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Codablock F | Stacked | High | Stacked Code 128/39 |
| Code 16k | Stacked | High | Stacked Code 128 |
| Code 49 | Stacked | High | Multi-row alphanumeric |
| GS1 DataBar Stacked | Stacked | Medium | Already partial via `stacked` option |
| GS1 DataBar Stacked Omnidirectional | Stacked | Medium | |
| GS1 DataBar Expanded Stacked | Stacked | Medium | Already partial via `stacked` option |

### Priority 4: 2D Matrix Codes

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Aztec Runes | Matrix | Medium | Small Aztec variant |
| Code One | Matrix | High | 2D matrix code |
| DotCode | Matrix | High | Dot-based matrix |
| Grid Matrix | Matrix | High | Chinese standard |
| Han Xin | Matrix | High | Chinese standard GB/T 21049 |
| UPNQR | Matrix | Low | Slovenian QR Code variant |

---

## Coding Conventions

### File Naming
- **Prefix convention by format category:**
  - `OD` = OneD (one-dimensional barcodes)
  - `AZ` = Aztec
  - `DM` = DataMatrix
  - `MC` = MaxiCode
  - `PDF` = PDF417
  - `QR` = QRCode
- **File structure:**
  - `{PREFIX}Reader.h/cpp` - Main reader class
  - `{PREFIX}Writer.h/cpp` - Main writer class
  - `{PREFIX}Decoder.h/cpp` - Decoding logic
  - `{PREFIX}Detector.h/cpp` - Pattern detection
  - `{PREFIX}Encoder.h/cpp` - Encoding logic

### Code Style (.clang-format)
```
Language: Cpp
Standard: c++17
IndentWidth: 4
TabWidth: 4
UseTab: ForContinuationAndIndentation
AccessModifierOffset: -4
ColumnLimit: 135
PointerAlignment: Left
BreakBeforeBraces: Custom (opening brace on new line for classes/functions)
SortIncludes: true
```

### Naming Conventions
- **Classes:** PascalCase (e.g., `Code128Reader`, `AZWriter`)
- **Methods:** camelCase (e.g., `decodePattern()`, `encode()`)
- **Constants:** UPPER_SNAKE_CASE (e.g., `CHARACTER_ENCODINGS`)
- **Member variables:** _prefixed (e.g., `_margin`, `_format`)

### Header Structure
```cpp
/*
* Copyright 2024 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ZXing/Header.h"

namespace ZXing::FormatName {

class Reader : public ZXing::Reader {
public:
    // ...
};

} // namespace ZXing::FormatName
```

---

## Implementation Patterns

### Directory Structure for New Format
```
core/src/newformat/
├── NFReader.h
├── NFReader.cpp
├── NFDecoder.h
├── NFDecoder.cpp
├── NFDetector.h (for 2D codes)
├── NFDetector.cpp
├── NFWriter.h
├── NFWriter.cpp
├── NFEncoder.h
├── NFEncoder.cpp
└── [Format-specific helpers]
```

### Reader Implementation Template

**Header (NFReader.h):**
```cpp
#pragma once

#include "Reader.h"

namespace ZXing::NewFormat {

class Reader : public ZXing::Reader
{
public:
    using ZXing::Reader::Reader;

    Barcode decode(const BinaryBitmap& image) const override;
    Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;
};

} // namespace ZXing::NewFormat
```

### Writer Implementation Template

**Header (NFWriter.h):**
```cpp
#pragma once

#include "BitMatrix.h"
#include "CharacterSet.h"

namespace ZXing::NewFormat {

class Writer
{
public:
    Writer& setMargin(int margin) { _margin = margin; return *this; }
    Writer& setEncoding(CharacterSet enc) { _encoding = enc; return *this; }

    BitMatrix encode(const std::string& contents, int width, int height) const;
    BitMatrix encode(const std::wstring& contents, int width, int height) const;

private:
    int _margin = -1;
    CharacterSet _encoding = CharacterSet::Unknown;
};

} // namespace ZXing::NewFormat
```

### BarcodeFormat Enum Addition

In `BarcodeFormat.h`:
```cpp
enum class BarcodeFormat
{
    // ... existing formats
    NewFormat = (1 << NEXT_AVAILABLE_BIT),  // Use next bit position after DataBarLimited (1 << 19)

    LinearCodes = /* ... add if linear */,
    MatrixCodes = /* ... add if matrix */,
};
```

In `BarcodeFormat.cpp`:
```cpp
static const BarcodeFormatName NAMES[] = {
    // ... existing
    {BarcodeFormat::NewFormat, "NewFormat"},
};
```

### CMake Integration

In `core/CMakeLists.txt`:
```cmake
# Add format option
option("ZXING_ENABLE_NEWFORMAT" "Enable support for NEWFORMAT barcodes" ON)

# Add to public flags
list(APPEND ZXING_PUBLIC_FLAGS
    $<$<BOOL:${ZXING_ENABLE_NEWFORMAT}>:-DZXING_WITH_NEWFORMAT>)

# Add reader files
if (ZXING_READERS AND ZXING_ENABLE_NEWFORMAT)
    set(NEWFORMAT_FILES ${NEWFORMAT_FILES}
        src/newformat/NFReader.h
        src/newformat/NFReader.cpp
        # ... other reader files
    )
endif()

# Add writer files
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_NEWFORMAT)
    set(NEWFORMAT_FILES ${NEWFORMAT_FILES}
        src/newformat/NFWriter.h
        src/newformat/NFWriter.cpp
        # ... other writer files
    )
endif()
```

### MultiFormatReader Registration

In `MultiFormatReader.cpp`:
```cpp
#ifdef ZXING_WITH_NEWFORMAT
if (formats.testFlag(BarcodeFormat::NewFormat))
    _readers.emplace_back(new NewFormat::Reader(opts));
#endif
```

### MultiFormatWriter Registration

In `MultiFormatWriter.cpp`:
```cpp
#ifdef ZXING_WITH_NEWFORMAT
case BarcodeFormat::NewFormat:
    return NewFormat::Writer().setMargin(margin).encode(contents, width, height);
#endif
```

### WriteBarcode.cpp (Zint Integration)

For NEW writer API integration with libzint:
```cpp
static constexpr BarcodeFormatZXing2Zint barcodeFormatZXing2Zint[] = {
    // ... existing
    {BarcodeFormat::NewFormat, BARCODE_NEWFORMAT},  // Use zint constant
};
```

---

## Wrapper Updates Required

When adding a new barcode format to the core library, the following wrapper APIs must also be updated:

### iOS Wrapper (Required)

The iOS wrapper requires updates to expose new formats to Swift/Objective-C applications.

**Files to update:**

1. **`wrappers/ios/Sources/Wrapper/ZXIFormat.h`**
   - Add new enum value to `ZXIFormat` typedef
   ```objc
   typedef NS_ENUM(NSInteger, ZXIFormat) {
       // ... existing formats
       AUSTRALIA_POST,  // Add new format
       // ...
   };
   ```

2. **`wrappers/ios/Sources/Wrapper/ZXIFormatHelper.mm`**
   - Add case to `BarcodeFormatFromZXIFormat()` function
   - Add case to `ZXIFormatFromBarcodeFormat()` function
   ```objc
   // In BarcodeFormatFromZXIFormat:
   case ZXIFormat::AUSTRALIA_POST:
       return ZXing::BarcodeFormat::AustraliaPost;

   // In ZXIFormatFromBarcodeFormat:
   case ZXing::BarcodeFormat::AustraliaPost:
       return ZXIFormat::AUSTRALIA_POST;
   ```

### Other Wrappers (As Needed)

The following wrappers may also need updates depending on how they expose format enums:

| Wrapper | Location | Update Required |
|---------|----------|-----------------|
| Android | `wrappers/android/` | Format enum mapping |
| Python | `wrappers/python/` | BarcodeFormat bindings |
| C API | `core/src/ZXingC.h` | ZXing_BarcodeFormat enum |
| .NET | `wrappers/dotnet/` | BarcodeFormat enum |
| Rust | `wrappers/rust/` | BarcodeFormat enum |
| WebAssembly | `wrappers/wasm/` | JavaScript bindings |

### C API Updates

When adding new formats, also update `core/src/ZXingC.h`:
```c
typedef enum {
    // ... existing
    ZXing_BarcodeFormat_AustraliaPost = (1 << 20),  // Next available bit
} ZXing_BarcodeFormat;
```

---

## Implementation Roadmap

### Phase 1: Low-Complexity Linear Codes
Start with simple 1D formats that follow established patterns:

1. **Code 11** - Simple linear, limited character set
2. **MSI (Modified Plessey)** - Simple linear with checksum
3. **Pharmacode** - Very simple linear (single track)
4. **Matrix 2 of 5** - Simple numeric-only code
5. **Industrial 2 of 5** - Similar to Matrix 2/5
6. **IATA 2 of 5** - Airline variant
7. **Datalogic 2 of 5** - Similar structure

### Phase 2: Code 39 Variants
These derive from existing Code39 implementation:

1. **LOGMARS** - Military variant of Code 39
2. **Code 32** - Italian pharmacode (base 32 encoding)
3. **Pharmazentralnummer** - German pharmaceutical

### Phase 3: Postal Codes
These share 4-state bar patterns:

1. **Royal Mail 4-State (RM4SCC)** - Base implementation
2. **Royal Mail 4-State Mailmark** - Extension of RM4SCC
3. **POSTNET / PLANET** - Simple height-based encoding
4. **Dutch Post KIX** - Similar to RM4SCC
5. **Australia Post** - 4 variants, bar-based
6. **Japan Post** - Similar pattern concept
7. **Korea Post** - Similar structure
8. **Deutsche Post Leitcode/Identcode** - German variants
9. **USPS Intelligent Mail** - Complex, combines POSTNET features

### Phase 4: Stacked Linear Codes
More complex multi-row formats:

1. **DataBar Stacked variants** - Extend existing DataBar
2. **Codablock F** - Stacked Code 128/39
3. **Code 16k** - Stacked Code 128
4. **Code 49** - Multi-row alphanumeric

### Phase 5: 2D Matrix Codes
Most complex implementations:

1. **Aztec Runes** - Small Aztec variant
2. **UPNQR** - QR Code variant
3. **DotCode** - Dot-based matrix
4. **Code One** - 2D matrix
5. **Grid Matrix** - Chinese standard
6. **Han Xin** - Chinese standard

---

## Testing Requirements

### Unit Tests
- Located in `test/unit/`
- Naming: `{Format}Test.cpp`
- Test encoding/decoding roundtrip
- Test edge cases and error handling

### Blackbox Tests
- Located in `test/blackbox/`
- Sample images for each format
- Test against real-world barcode samples

### Build Configuration Test
```bash
# Test with all formats enabled
cmake -B build -DZXING_READERS=ON -DZXING_WRITERS=BOTH

# Test with specific format disabled
cmake -B build -DZXING_ENABLE_NEWFORMAT=OFF
```

---

## Progress Tracking

### Completed
- [x] **Australia Post** (6 FCC variants) - Fully implemented with:
  - 4-state bar detection with height analysis (Full, Ascender, Descender, Tracker)
  - N-Table (numeric) and C-Table (alphanumeric) encoding/decoding
  - Reed-Solomon GF(64) error correction
  - FCC 11 (Standard Customer), 45 (Reply Paid), 59 (Customer Barcode 2), 62 (Customer Barcode 3), 87 (Routing), 92 (Redirection)
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **Dutch Post KIX Code** - Fully implemented with:
  - 4-state bar detection with height analysis (Full, Ascender, Descender, Tracker)
  - Same encoding as Royal Mail 4-State (RM4SCC) but without start/stop bars and checksum
  - Royal Table encoding for alphanumeric characters (0-9, A-Z)
  - 7-24 character support (28-96 bars)
  - Bidirectional decoding (can read upside-down barcodes)
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **Japan Post 4-State** (Kasutama Barcode) - Fully implemented with:
  - 4-state bar detection with height analysis (Full=4, Ascender=2, Descender=3, Tracker=1)
  - 3-bar character encoding for digits (0-9), hyphen (-), and control codes
  - Letter encoding via control code pairs (CC1+digit=A-J, CC2+digit=K-T, CC3+digit=U-Z)
  - Fixed 67-bar structure: Start(2) + PostalCode(21) + Address(39) + CheckDigit(3) + Stop(2)
  - Modulo 19 checksum validation
  - Bidirectional decoding (can read upside-down barcodes)
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **Korea Post** (Korean Postal Authority Code) - Fully implemented with:
  - 6-digit postal code encoding with modulo-10 check digit
  - Variable-width bar/space patterns for each digit (0-9)
  - Pattern matching with tolerance-based variance calculation
  - Check digit validation (10 - sum % 10)
  - Simple linear barcode (not 4-state) using RowReader pattern
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **RM4SCC** (Royal Mail 4-State Customer Code) - Fully implemented with:
  - 4-state bar detection with height analysis (Full, Ascender, Descender, Tracker)
  - Same Royal Table encoding as KIX Code (36 characters: 0-9, A-Z)
  - Start bar (Ascender) and Stop bar (Full) detection
  - Modulo-6 checksum validation (row/column sum algorithm)
  - UK postcode + Delivery Point Suffix (DPS) support
  - Bidirectional decoding (can read upside-down barcodes)
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **Mailmark** (Royal Mail 4-State Mailmark) - Fully implemented with:
  - Two variants: Barcode C (22 chars, 66 bars) and Barcode L (26 chars, 78 bars)
  - 4-state bar detection with DAFT encoding (Descender, Ascender, Full, Tracker)
  - 3-bar extender groups with position-dependent parity encoding
  - Reed-Solomon GF(64) error correction with custom polynomial (x^6+x^5+x^2+1)
  - Barcode C: 16 data symbols + 6 check symbols
  - Barcode L: 19 data symbols + 7 check symbols
  - CDV (Consolidated Data Value) big integer extraction for field decoding
  - Data fields: Format, Version ID, Mail Class, Supply Chain ID, Item ID, Postcode+DPS
  - Bidirectional decoding support
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **USPSIMB** (USPS Intelligent Mail Barcode / OneCode / 4CB) - Fully implemented with:
  - 65-bar 4-state barcode (Full, Ascender, Descender, Tracker)
  - 130 total bits (each bar encodes ascender bit + descender bit)
  - 10 codewords of 13 bits each extracted via mixed radix conversion
  - 5-of-13 character table (1287 entries) and 2-of-13 character table (78 entries)
  - CRC-11 error detection (polynomial 0x0F35, init 0x07FF)
  - Bar-to-character and bar-to-bit mapping tables for decoding
  - Data fields: Barcode ID (2), Service Type ID (3), Mailer ID (6/9), Serial (9/6), Routing (0/5/9/11 digits)
  - Bidirectional decoding support
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **DeutschePostLeitcode** (Deutsche Post Leitcode) - Fully implemented with:
  - 14-digit ITF-based barcode for German mail routing
  - Structure: PLZ (5 digits) + Street (3 digits) + House (3 digits) + Product (2 digits) + Check (1 digit)
  - Modulo 10 check digit with weights 4 and 9 (alternating)
  - Based on Interleaved 2 of 5 (ITF) encoding
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **DeutschePostIdentcode** (Deutsche Post Identcode) - Fully implemented with:
  - 12-digit ITF-based barcode for German shipment identification
  - Structure: MailCenter (2 digits) + Customer (3 digits) + Delivery (6 digits) + Check (1 digit)
  - Same check digit algorithm as Leitcode (Modulo 10 with weights 4 and 9)
  - Based on Interleaved 2 of 5 (ITF) encoding
  - Shares reader implementation with Leitcode (ODDeutschePostReader)
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **Code11** (Code 11 / USD-8) - Fully implemented with:
  - Character set: digits 0-9 and dash (-)
  - Each character: 3 bars + 2 spaces (5 elements)
  - Start/stop character pattern
  - Check digit C: Modulo 11 with weights 1-10 cycling from right
  - Check digit K: Modulo 11 with weights 1-9 cycling from right (includes C)
  - Single check digit (C) for data <= 10 characters
  - Dual check digits (C + K) for data > 10 characters
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **POSTNET** (USPS Postal Numeric Encoding Technique) - Fully implemented with:
  - Height-modulated barcode (tall=1, short=0)
  - 5-bar encoding per digit with weights 7-4-2-1-0
  - 2 tall bars (plus optional third for zero) per digit
  - Frame bars (tall bars at start and end)
  - Modulo 10 check digit
  - Supports ZIP (32 bars), ZIP+4 (52 bars), ZIP+4+DPC (62 bars)
  - Bidirectional decoding support
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

- [x] **PLANET** (USPS Postal Alpha Numeric Encoding Technique) - Fully implemented with:
  - Height-modulated barcode (inverse of POSTNET: tall=0, short=1)
  - 5-bar encoding per digit with weights 7-4-2-1-0
  - 3 tall bars (or 2 for zero) per digit
  - Frame bars (tall bars at start and end)
  - Modulo 10 check digit
  - Supports 12-digit (62 bars) and 14-digit (72 bars) formats
  - Bidirectional decoding support
  - Shares reader implementation with POSTNET (ODPOSTNETReader)
  - Writer available via libzint integration (ZXING_WRITERS=NEW)
  - iOS wrapper and C API updated

### Pending Phases
- [ ] Phase 1: Low-Complexity Linear Codes
- [ ] Phase 2: Code 39 Variants
- [ ] Phase 3: Postal Codes (Australia Post first)
- [ ] Phase 4: Stacked Linear Codes
- [ ] Phase 5: 2D Matrix Codes

### Implementation Checklist Template
For each new format, complete these steps:
- [ ] Add to `BarcodeFormat.h` enum
- [ ] Add to `BarcodeFormat.cpp` NAMES array
- [ ] Create reader implementation (`core/src/{format}/`)
- [ ] Create writer implementation (if applicable)
- [ ] Register in `MultiFormatReader.cpp`
- [ ] Register in `MultiFormatWriter.cpp` (if applicable)
- [ ] Update `core/CMakeLists.txt`
- [ ] Update iOS wrapper (`ZXIFormat.h`, `ZXIFormatHelper.mm`)
- [ ] Update C API (`ZXingC.h`)
- [ ] Add unit tests
- [ ] Update this documentation

---

## References

- [ZXing-C++ GitHub](https://github.com/zxing-cpp/zxing-cpp)
- [Zint Barcode Generator](https://github.com/zint/zint) - For format specifications
- [GS1 Standards](https://www.gs1.org/) - DataBar specifications
- [ISO/IEC 15438](https://www.iso.org/) - PDF417 specification
- [ISO/IEC 16022](https://www.iso.org/) - DataMatrix specification
- [ISO/IEC 18004](https://www.iso.org/) - QR Code specification
