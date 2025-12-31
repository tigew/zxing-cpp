/*
* Copyright 2025 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ODRowReader.h"

namespace ZXing::OneD {

/**
 * Deutsche Post barcode reader for Leitcode and Identcode.
 *
 * Both formats are based on Interleaved 2 of 5 (ITF) encoding with a custom
 * check digit algorithm using weights 4 and 9.
 *
 * Leitcode (14 digits):
 * - Used for mail routing (Leitweg)
 * - Structure: PLZ(5) + Street(3) + House(3) + Product(2) + Check(1)
 * - PLZ: 5-digit postal code
 * - Street: 3-digit street identifier
 * - House: 3-digit house number
 * - Product: 2-digit product code (00=parcel, 33=return, etc.)
 * - Check: 1-digit check digit
 *
 * Identcode (12 digits):
 * - Used for shipment identification
 * - Structure: MailCenter(2) + Customer(3) + Delivery(6) + Check(1)
 * - MailCenter: 2-digit distribution center ID
 * - Customer: 3-digit customer code
 * - Delivery: 6-digit delivery identifier
 * - Check: 1-digit check digit
 *
 * Check digit algorithm:
 * - Multiply each digit by alternating weights 4, 9, 4, 9, ...
 * - Sum all products
 * - Check digit = (10 - (sum mod 10)) mod 10
 */
class DeutschePostReader : public RowReader
{
public:
	using RowReader::RowReader;

	Barcode decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
};

} // namespace ZXing::OneD
