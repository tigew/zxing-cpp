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

---

## Current Implementation Status

### Fully Supported Formats (21 formats)

| Format | Read | Write (OLD) | Write (NEW/Zint) | Notes |
|--------|------|-------------|------------------|-------|
| AustraliaPost | Yes | No | Yes | 4-state postal, 6 FCC variants (11, 45, 59, 62, 87, 92) |
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

## Types of Symbology (Zint-aligned)

The list below mirrors the Zint manual’s structure and is used to plan missing format implementations. Each entry
includes a complexity estimate and notes to guide future work.

### 6.1 One-Dimensional Symbols
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| Code 11 | — | Low | Telecommunications |
| Code 2 of 5 | Standard, IATA, Industrial, Interleaved (ISO 16390), Data Logic, ITF-14, Deutsche Post Leitcode/Identcode | Low | 2 of 5 family |
| UPC (ISO 15420) | UPC-A, UPC-E | Low | Retail UPC |
| EAN (ISO 15420) | EAN-2, EAN-5, EAN-8, EAN-13, SBN/ISBN/ISBN-13 | Low | Retail EAN |
| Plessey | UK Plessey, MSI Plessey | Medium | Inventory/warehousing |
| Telepen | Alpha, Numeric | Medium | UK library system |
| Code 39 (ISO 16388) | Standard, Extended, Code 93, PZN, LOGMARS, Code 32, HIBC Code 39, VIN | Low | Code 39 family |
| Codabar (EN 798) | — | Low | Legacy |
| Pharmacode | — | Low | Pharmaceutical |
| Code 128 (ISO 15417) | Standard, Subset B, GS1-128, EAN-14, NVE-18, HIBC, DPD | Medium | High density |
| GS1 DataBar (ISO 24724) | Omnidirectional/Truncated, Limited, Expanded | Medium | GS1 family |
| Korea Post Barcode | — | Medium | Postal linear |
| Channel Code | — | Medium | Compact numeric encoding |

### 6.2 Stacked Symbologies
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| Codablock-F | — | High | Stacked Code 128/39 |
| Code 16K (EN 12323) | — | High | Stacked Code 128 |
| PDF417 (ISO 15438) | Compact PDF417, MicroPDF417 (ISO 24728) | Medium | Stacked linear |
| GS1 DataBar Stacked (ISO 24724) | Stacked, Stacked Omnidirectional, Expanded Stacked | Medium | GS1 variants |
| Code 49 | — | High | Multi-row alphanumeric |

### 6.3 GS1 Composite Symbols (ISO 24723)
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| GS1 Composite | CC-A, CC-B, CC-C | High | Composite components |

### 6.4 Two-Track Symbols
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| Two-Track Pharmacode | — | Medium | Pharmaceutical |
| POSTNET | — | Low | Legacy US postal |
| PLANET | — | Low | Legacy US postal |

### 6.5 4-State Postal Codes
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| Australia Post 4-State | Customer, Reply Paid, Routing, Redirect | Medium | 4-state postal |
| Dutch Post KIX | — | Medium | Netherlands postal |
| Royal Mail 4-State (RM4SCC) | — | Medium | UK postal |
| Royal Mail Mailmark | — | High | UK postal |
| USPS Intelligent Mail | — | High | US postal |
| Japanese Postal Code | — | Medium | JP postal |
| DAFT Code | — | Medium | French postal |

### 6.6 Matrix Symbols
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| Data Matrix (ISO 16022) | — | Medium | ECC200 |
| QR Code (ISO 18004) | — | Medium | QR family |
| Micro QR Code (ISO 18004) | — | Medium | QR variant |
| Rectangular Micro QR Code (rMQR) (ISO 23941) | — | Medium | QR variant |
| UPNQR | — | Low | Slovenian QR variant |
| MaxiCode (ISO 16023) | — | High | Shipping |
| Aztec Code (ISO 24778) | — | Medium | Aztec |
| Aztec Runes (ISO 24778) | — | Low | Small Aztec |
| Code One | — | High | 2D matrix code |
| Grid Matrix | — | High | Chinese standard |
| DotCode | — | High | Dot-based matrix |
| Han Xin Code (ISO 20830) | — | High | Chinese standard |
| Ultracode | — | High | Matrix hybrid |

### 6.7 Other Barcode-Like Markings
| Format | Variants | Complexity | Notes |
|--------|----------|------------|-------|
| Facing Identification Mark (FIM) | — | Low | USPS |
| Flattermarken | — | Low | Legacy |

### Reader Folder Structure (Zint-aligned)
Match reader/writer placement to the symbology family:
- `core/src/oned/` → One-Dimensional Symbols
- `core/src/stacked/` → Stacked Symbologies (e.g., `stacked/pdf417/`)
- `core/src/stacked/gs1/` → GS1 helpers (e.g., GTIN, HRI)
- `core/src/composite/` → GS1 Composite Symbols
- `core/src/twotrack/` → Two-Track Symbols
- `core/src/postal/` → 4-State Postal Codes
- `core/src/matrix/` → Matrix Symbols (e.g., `matrix/aztec/`, `matrix/qrcode/`, `matrix/datamatrix/`, `matrix/maxicode/`)
- `core/src/other/` → Other Barcode-Like Markings
- `core/src/reader/` → Reader orchestration and public reader APIs

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

#include "reader/Reader.h"

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
# Master enable flag (global)
option("ZXING_ENABLE_ALL" "Enable all barcode families" ON)

# Category-level enable flags
option("ZXING_ENABLE_CATEGORY_ONED" "Enable One-Dimensional symbols" ON)
option("ZXING_ENABLE_CATEGORY_STACKED" "Enable Stacked symbologies" ON)
option("ZXING_ENABLE_CATEGORY_COMPOSITE" "Enable GS1 composite symbols" ON)
option("ZXING_ENABLE_CATEGORY_TWOTRACK" "Enable Two-Track symbols" ON)
option("ZXING_ENABLE_CATEGORY_POSTAL" "Enable 4-State postal symbols" ON)
option("ZXING_ENABLE_CATEGORY_MATRIX" "Enable Matrix symbologies" ON)
option("ZXING_ENABLE_CATEGORY_OTHER" "Enable other barcode-like markings" ON)

# Format-level flag (single reader)
option("ZXING_ENABLE_NEWFORMAT" "Enable support for NEWFORMAT barcodes" ON)

# Variant-level flags (when a format has multiple variants)
option("ZXING_ENABLE_NEWFORMAT_VARIANT_X" "Enable NEWFORMAT Variant X" ON)
option("ZXING_ENABLE_NEWFORMAT_VARIANT_Y" "Enable NEWFORMAT Variant Y" ON)

# Effective enablement (master or category + format/variant)
function (zxing_apply_category out_var category_var)
    if (ZXING_ENABLE_ALL OR ${category_var})
        set (${out_var} ON PARENT_SCOPE)
    else()
        set (${out_var} OFF PARENT_SCOPE)
    endif()
endfunction()

function (zxing_apply_enable out_var category_var format_var)
    if (ZXING_ENABLE_ALL OR (${category_var} AND ${format_var}))
        set (${out_var} ON PARENT_SCOPE)
    else()
        set (${out_var} OFF PARENT_SCOPE)
    endif()
endfunction()

zxing_apply_category(ZXING_ENABLE_CATEGORY_MATRIX_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX)
zxing_apply_enable(ZXING_ENABLE_NEWFORMAT_EFFECTIVE ZXING_ENABLE_CATEGORY_MATRIX_EFFECTIVE ZXING_ENABLE_NEWFORMAT)

# Add to public flags
list(APPEND ZXING_PUBLIC_FLAGS
    $<$<BOOL:${ZXING_ENABLE_NEWFORMAT_EFFECTIVE}>:-DZXING_WITH_NEWFORMAT>)

# Add reader files
if (ZXING_READERS AND ZXING_ENABLE_NEWFORMAT_EFFECTIVE)
    set(NEWFORMAT_FILES ${NEWFORMAT_FILES}
        src/newformat/NFReader.h
        src/newformat/NFReader.cpp
        # ... other reader files
    )
endif()

# Add writer files
if (ZXING_WRITERS_OLD AND ZXING_ENABLE_NEWFORMAT_EFFECTIVE)
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

## Implementation Roadmap (Zint-aligned)

Use the symbology categories above as the primary roadmap grouping. Prioritize within each family based on demand,
implementation complexity, and available test assets:

1. **One-Dimensional Symbols** (basic linear codes and variants)
2. **Stacked Symbologies**
3. **GS1 Composite Symbols**
4. **Two-Track Symbols**
5. **4-State Postal Codes**
6. **Matrix Symbols**
7. **Other Barcode-Like Markings**

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

### Pending Families
- [ ] One-Dimensional Symbols
- [ ] Stacked Symbologies
- [ ] GS1 Composite Symbols
- [ ] Two-Track Symbols
- [ ] 4-State Postal Codes
- [ ] Matrix Symbols
- [ ] Other Barcode-Like Markings

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
