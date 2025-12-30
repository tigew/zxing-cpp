/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Reader.h"

namespace ZXing {

class BitMatrix;

namespace OneD {

/**
 * @brief Reader for Dutch Post KIX Code 4-State barcodes.
 *
 * KIX (Klantenindex) is used by Royal Dutch TPG Post (Netherlands) for
 * postal code and automatic mail sorting. It uses the same encoding as
 * Royal Mail 4-State (RM4SCC) but without start/stop bars and checksum.
 *
 * Technical specifications:
 * - 4-state bars: Full (F), Ascender (A), Descender (D), Tracker (T)
 * - Character set: 0-9, A-Z (36 characters)
 * - Each character encoded as 4 bars
 * - Length: 7-24 characters (28-96 bars)
 * - No start/stop bars or checksum (unlike RM4SCC)
 */
class KIXCodeReader : public ZXing::Reader
{
public:
	explicit KIXCodeReader(const ReaderOptions& opts) : ZXing::Reader(opts) {}

	Barcode decode(const BinaryBitmap& image) const override;
	Barcodes decode(const BinaryBitmap& image, int maxSymbols) const override;

private:
	Barcode decodeInternal(const BitMatrix& image, bool tryRotated) const;
};

} // namespace OneD
} // namespace ZXing
