// Copyright 2022 KURZ Digital Solutions GmbH
//
// SPDX-License-Identifier: Apache-2.0

#import <Foundation/Foundation.h>
#ifndef ZXIFormat_h
#define ZXIFormat_h

// Keep in sync with core/src/BarcodeFormat.h.
typedef NS_ENUM(NSInteger, ZXIFormat) {
    NONE,
    AUSTRALIA_POST,
    AZTEC,
    AZTEC_RUNE,
    CODABAR,
    CODE_11,
    CODE_39,
    CODE_93,
    CODE_128,
    CODE_ONE,
    DATA_BAR,
    DATA_BAR_EXPANDED,
    DATA_BAR_LIMITED,
    DATA_BAR_STACKED,
    DATA_BAR_STACKED_OMNIDIRECTIONAL,
    DATA_BAR_EXPANDED_STACKED,
    DATA_MATRIX,
    DOT_CODE,
    GRID_MATRIX,
    HAN_XIN,
    DX_FILM_EDGE,
    EAN_8,
    EAN_13,
    ITF,
    JAPAN_POST,
    KIX_CODE,
    KOREA_POST,
    MAILMARK,
    RM4SCC,
    USPS_IMB,
    DEUTSCHE_POST_LEITCODE,
    DEUTSCHE_POST_IDENTCODE,
    POSTNET,
    PLANET,
    MSI,
    TELEPEN,
    LOGMARS,
    CODE_32,
    PHARMACODE,
    PHARMACODE_TWO_TRACK,
    PZN,
    CHANNEL_CODE,
    MATRIX_2_OF_5,
    INDUSTRIAL_2_OF_5,
    IATA_2_OF_5,
    DATALOGIC_2_OF_5,
    CODABLOCK_F,
    CODE_16K,
    CODE_49,
    MAXICODE,
    PDF_417,
    QR_CODE,
    MICRO_QR_CODE,
    RMQR_CODE,
    UPN_QR,
    UPC_A,
    UPC_E,
    LINEAR_CODES,
    MATRIX_CODES,
    ANY
};

#endif 
