// Copyright 2022 KURZ Digital Solutions GmbH
//
// SPDX-License-Identifier: Apache-2.0

#import <Foundation/Foundation.h>
#ifndef ZXIFormat_h
#define ZXIFormat_h

typedef NS_ENUM(NSInteger, ZXIFormat) {
    NONE = 0,
    AZTEC = 1,
    CODABAR = 2,
    CODE_39 = 3,
    CODE_93 = 4,
    CODE_128 = 5,
    DATA_BAR = 6,
    DATA_BAR_EXPANDED = 7,
    DATA_BAR_LIMITED = 8,
    DATA_MATRIX = 9,
    DX_FILM_EDGE = 10,
    EAN_8 = 11,
    EAN_13 = 12,
    ITF = 13,
    MAXICODE = 14,
    PDF_417 = 15,
    QR_CODE = 16,
    MICRO_QR_CODE = 17,
    RMQR_CODE = 18,
    UPC_A = 19,
    UPC_E = 20,
    LINEAR_CODES = 21,
    MATRIX_CODES = 22,
    ANY = 23,
    AUSTRALIA_POST = 24
};

#endif 
