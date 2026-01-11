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
    CHANNEL_CODE,
    CODABAR,
    CODABLOCK_F,
    CODE_11,
    CODE_128,
    CODE_16K,
    CODE_32,
    CODE_39,
    CODE_49,
    CODE_93,
    CODE_ONE,
    DATA_BAR,
    DATA_BAR_EXPANDED,
    DATA_BAR_EXPANDED_STACKED,
    DATA_BAR_LIMITED,
    DATA_BAR_STACKED,
    DATA_BAR_STACKED_OMNIDIRECTIONAL,
    DATALOGIC_2_OF_5,
    DATA_MATRIX,
    DEUTSCHE_POST_IDENTCODE,
    DEUTSCHE_POST_LEITCODE,
    DOTCODE,
    DX_FILM_EDGE,
    EAN_13,
    EAN_8,
    GRID_MATRIX,
    HAN_XIN,
    IATA_2_OF_5,
    INDUSTRIAL_2_OF_5,
    ITF,
    JAPAN_POST,
    KIX_CODE,
    KOREA_POST,
    LOGMARS,
    MAILMARK,
    MATRIX_2_OF_5,
    MAXICODE,
    MICRO_QR_CODE,
    MSI,
    PDF_417,
    PHARMACODE,
    PHARMACODE_TWO_TRACK,
    PLANET,
    POSTNET,
    PZN,
    QR_CODE,
    RM4SCC,
    RMQR_CODE,
    TELEPEN,
    UPC_A,
    UPC_E,
    UPNQR,
    USPS_IMB,
    LINEAR_CODES,
    MATRIX_CODES,
    ANY
};

#endif 
