# ZXing-C++ Barcode Format Implementation Guide

This document tracks the implementation status of barcode formats in zxing-cpp and provides guidelines for adding new format support.

## Table of Contents
- [Current Implementation Status](#current-implementation-status)
- [Missing Formats to Implement](#missing-formats-to-implement)
- [Coding Conventions](#coding-conventions)
- [Implementation Patterns](#implementation-patterns)
- [Implementation Roadmap](#implementation-roadmap)

---

## Current Implementation Status

### Fully Supported Formats (20 formats)

| Format | Read | Write (OLD) | Write (NEW/Zint) | Notes |
|--------|------|-------------|------------------|-------|
| Aztec | Yes | Yes | Yes | Full support |
| Codabar | Yes | Yes | Yes | |
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
| MaxiCode | Yes (partial) | No | Yes | |
| MicroQRCode | Yes | No | Yes | |
| PDF417 | Yes | Yes | Yes | Includes Truncated variant |
| QRCode | Yes | Yes | Yes | |
| RMQRCode | Yes | No | Yes | Rectangular Micro QR Code |
| UPC-A | Yes | Yes | Yes | |
| UPC-E | Yes | Yes | Yes | |

---

## Missing Formats to Implement

### Priority 1: Postal Codes (High demand)

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Australia Post (4 variants) | Postal | Medium | Standard Customer, Reply Paid, Routing, Redirection |
| Dutch Post KIX Code | Postal | Medium | Netherlands postal |
| Japan Post | Postal | Medium | |
| Korea Post | Postal | Medium | |
| Royal Mail 4-State (RM4SCC) | Postal | Medium | UK postal |
| Royal Mail 4-State Mailmark | Postal | Medium | Modern UK postal |
| USPS OneCode (Intelligent Mail) | Postal | High | US postal |
| POSTNET / PLANET | Postal | Low | Legacy US postal |
| Deutsche Post Leitcode | Postal | Medium | German routing |
| Deutsche Post Identcode | Postal | Medium | German identification |

### Priority 2: Industrial/Linear Codes

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Code 11 | Linear | Low | Telecommunications |
| MSI (Modified Plessey) | Linear | Low | Inventory/warehousing |
| Telepen | Linear | Low | UK library system |
| LOGMARS | Linear | Low | Code 39 variant for military |
| Code 32 | Linear | Low | Italian Pharmacode (Code 39 variant) |
| Pharmacode | Linear | Low | Pharmaceutical |
| Pharmacode Two-Track | Linear | Medium | Pharmaceutical |
| Pharmazentralnummer | Linear | Low | German pharmaceutical (Code 39 variant) |
| Channel Code | Linear | Medium | Compact numeric encoding |

### Priority 3: Code 2 of 5 Variants

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Matrix 2 of 5 | Linear | Low | Also known as Code 25 |
| Industrial 2 of 5 | Linear | Low | |
| IATA 2 of 5 | Linear | Low | Airline industry |
| Datalogic 2 of 5 | Linear | Low | |

### Priority 4: Stacked/Multi-Row Linear

| Format | Category | Complexity | Notes |
|--------|----------|------------|-------|
| Codablock F | Stacked | High | Stacked Code 128/39 |
| Code 16k | Stacked | High | Stacked Code 128 |
| Code 49 | Stacked | High | Multi-row alphanumeric |
| GS1 DataBar Stacked | Stacked | Medium | Already partial via `stacked` option |
| GS1 DataBar Stacked Omnidirectional | Stacked | Medium | |
| GS1 DataBar Expanded Stacked | Stacked | Medium | Already partial via `stacked` option |

### Priority 5: 2D Matrix Codes

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
- [x] Initial documentation created
- [ ] Phase 1: Low-Complexity Linear Codes
- [ ] Phase 2: Code 39 Variants
- [ ] Phase 3: Postal Codes
- [ ] Phase 4: Stacked Linear Codes
- [ ] Phase 5: 2D Matrix Codes

### Next Steps
1. Start with Code 11 implementation (simplest missing format)
2. Follow the implementation pattern documented above
3. Add unit tests for each new format
4. Update this document as formats are completed

---

## References

- [ZXing-C++ GitHub](https://github.com/zxing-cpp/zxing-cpp)
- [Zint Barcode Generator](https://github.com/zint/zint) - For format specifications
- [GS1 Standards](https://www.gs1.org/) - DataBar specifications
- [ISO/IEC 15438](https://www.iso.org/) - PDF417 specification
- [ISO/IEC 16022](https://www.iso.org/) - DataMatrix specification
- [ISO/IEC 18004](https://www.iso.org/) - QR Code specification
