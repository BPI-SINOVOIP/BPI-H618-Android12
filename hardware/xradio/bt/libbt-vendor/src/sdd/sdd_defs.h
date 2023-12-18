/******************************************************************************
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 ******************************************************************************/


#ifndef _SDD_DEFS_H_
#define _SDD_DEFS_H_


/***************************************************************************
 * Definitions of IE's from SDD file definition
 ***************************************************************************/


/* Version of SDD definition this is taken from */
#define SDD_VER_00_01                   0x0001

/*----------------------------------------------------------------------*/
/* SDD Section ID Definitions (Ref. 6.2)                                */
/*----------------------------------------------------------------------*/
#define SDD_WLAN_STATIC_SECT_ID         0x00
#define SDD_WLAN_DYNAMIC_SECT_ID        0x01
#define SDD_WLAN_PSVP_PHY_SECT_ID       0x02
#define SDD_WLAN_PHY_SECT_ID            0x03
#define SDD_WLAN_DEBUG_SECT_ID          0x04
#define SDD_BT_EFUSE_SECT_ID            0x14
#define SDD_LAST_SECT_ID                0xFF

/*----------------------------------------------------------------------*/
/* SDD General Element Types ID Definitions (Ref. 6.1)                  */
/*----------------------------------------------------------------------*/
#define SDD_VERSION_ELT_ID                      0xC0
#define SDD_SECTION_HEADER_ELT_ID               0xC1
#define SDD_END_OF_CONTENT_ELT_ID               0xFE

/*----------------------------------------------------------------------*/
/* SDD WLAN Static Section Element Type Definitions (Ref. 6.3.1)        */
/*----------------------------------------------------------------------*/
#define SDD_CW1200_PIN_CONFIG_ELT_ID            0xC4
#define SDD_REFERENCE_FREQUENCY_ELT_ID          0xC5
#define SDD_WAKEUP_MARGIN_ELT_ID                0xE9
#define SDD_SMPS_ELT_ID                         0xEA
#define SDD_REF_CLK_STARTUP_TIME_ELT_ID         0xC7
#define SDD_COUNTRY_INFO_ELT_ID                 0xDF
#define SDD_PTA_CFG_ELT_ID                      0xEB
#define SDD_HIF_PIN_CONFIG_ELD_ID               0xEE

/*----------------------------------------------------------------------*/
/* SDD WLAN Dynamic Section Element Type Definitions (Ref. 6.3.2)       */
/*----------------------------------------------------------------------*/
#define SDD_WLAN_MAC_ADDRESS_ELT_ID             0xC8
#define SDD_XTAL_TRIM_ELT_ID                    0xC9
#define SDD_REF_CLK_OFFSET_ELT_ID               0xCA

/*----------------------------------------------------------------------*/
/* SDD WLAN PSVP PHY Section Element Type Definitions (Ref. 6.3.3)      */
/*----------------------------------------------------------------------*/
#define SDD_PSVP_RADIO_SERIAL_NUMBER_ELT_ID     0xCB
#define SDD_WLAN_PSVP_2G4_TX_CAL_ELT_ID         0xCC
#define SDD_WLAN_PSVP_5G_TX_CAL_ELT_ID          0xCD
#define SDD_WLAN_PSVP_AGC_ELT_ID                0xCE

/*----------------------------------------------------------------------*/
/* SDD WLAN PHY Section Element Type Definitions (Ref. 4.9)           */
/*----------------------------------------------------------------------*/
#define SDD_RADIO_SERIAL_NUMBER_ELT_ID          0xCF
#define SDD_RSSI_COR_2G4_ELT_ID                 0xE0
#define SDD_RSSI_COR_5G_ELT_ID                  0xE1
#define SDD_MAX_OUTPUT_POWER_2G4_ELT_ID         0xE3
#define SDD_MAX_OUTPUT_POWER_5G_ELT_ID          0xE4
#define SDD_PA_ERR_SMOOTHING_ELT_ID             0xE5
#define SDD_RADIO_SUB_TYPE_ELT_ID                0xE8


#if (SDD_NEW_PWR_CTRL == 1)
/*****************************************************/
#define SDD_CH_MAX_OUTPUT_POWER_2G4_ELT_ID      0xEC
#define SDD_CH_MAX_OUTPUT_POWER_5G_ELT_ID       0xED
#define SDD_POWER_BO_VOLT_2G4_ELT_ID            0x20
#define SDD_POWER_BO_VOLT_5G_ELT_ID             0x21
#define SDD_POWER_BO_TEMP_2G4_ELT_ID            0x22
#define SDD_POWER_BO_TEMP_5G_ELT_ID             0x23
#define SDD_FE_COR_2G4_ELT_ID                   0x30
#define SDD_FE_COR_5G_ELT_ID                    0x31
#define SDD_PA_GAIN_CHAR_2G4_ELT_ID             0x40
#define SDD_PA_GAIN_CHAR_5G_ELT_ID              0x41
#define SDD_PA_GAIN_TEMP_CORR_2G4_ELT_ID        0x42
#define SDD_PA_GAIN_TEMP_CORR_5G_ELT_ID         0x43
#define SDD_PA_GAIN_VOLT_CORR_2G4_ELT_ID        0x44
#define SDD_PA_GAIN_VOLT_CORR_5G_ELT_ID         0x45
#define SDD_PA_GAIN_FREQ_CORR_2G4_ELT_ID        0x46
#define SDD_PA_GAIN_FREQ_CORR_5G_ELT_ID         0x47
#define SDD_PA_MAX_DIGITAL_GAIN_2G4_ELT_ID      0x48
#define SDD_PA_MAX_DIGITAL_GAIN_5G_ELT_ID       0x49
#define SDD_PA_DET_CHAR_2G4_ELT_ID              0x50
#define SDD_PA_DET_CHAR_5G_ELT_ID               0x51
#define SDD_PA_DET_FREQ_CORR_2G4_ELT_ID         0x52
#define SDD_PA_DET_FREQ_CORR_5G_ELT_ID          0x53
#define SDD_PA_DET_TEMP_CORR_2G4_ELT_ID         0x54
#define SDD_PA_DET_TEMP_CORR_5G_ELT_ID          0x55
#define SDD_PA_DET_VOLT_CORR_2G4_ELT_ID         0x56
#define SDD_PA_DET_VOLT_CORR_5G_ELT_ID          0x57
#define SDD_FEM_CONTROL_TABLE_ELT_ID            0xC6
#define SDD_SET_TX_CALIB_PARAMS_ELT_ID            0xEF
/*****************************************************/
#else //(SDD_NEW_PWR_CTRL == 1)
/*****************************************************/
#define SDD_PA_OPEN_LOOP_OUTPUT_SETTING_5G_ELT_ID   0xE2
#define SDD_PA_MAX_DIGITAL_GAIN_2G4_ELT_ID      0xE6
#define SDD_PA_MAX_DIGITAL_GAIN_5G_ELT_ID       0xE7
#define SDD_PA_OPEN_LOOP_OUTPUT_SETTING_2G4_ELT_ID  0xD0
#define SDD_PA_OPEN_LOOP_FREQ_CORR_2G4_ELT_ID   0xD1
#define SDD_PA_OPEN_LOOP_FREQ_CORR_5G_ELT_ID    0xD2
#define SDD_PA_DET_CHAR_2G4_ELT_ID              0xD3
#define SDD_PA_DET_CHAR_5G_ELT_ID               0xD4
#define SDD_PA_DET_OFFSET_2G4_ELT_ID            0xD5
#define SDD_PA_DET_OFFSET_5G_ELT_ID             0xD6
#define SDD_PA_DET_FREQ_CORR_2G4_ELT_ID         0xD7
#define SDD_PA_DET_FREQ_CORR_5G_ELT_ID          0xD8
#define SDD_PA_DET_TEMP_CORR_2G4_ELT_ID         0xD9
#define SDD_PA_DET_TEMP_CORR_5G_ELT_ID          0xDA
#define SDD_PA_DET_VOLT_CORR_2G4_ELT_ID         0xDB
#define SDD_PA_DET_VOLT_CORR_5G_ELT_ID          0xDC
#define SDD_FEM_CONTROL_TABLE_ELT_ID            0xC6
/*****************************************************/
#endif


#define SDD_POWER_CTRL_STEP_DBM_ELT_ID            0xAA

/*----------------------------------------------------------------------*/
/* SDD WLAN Debug Section Element Type Definitions (Ref. 4.10)          */
/*----------------------------------------------------------------------*/
#define SDD_SET_REGISTER_ELT_ID                 0xDD
#define SDD_SET_DEBUG_OUTPUT_ELT_ID             0xDE

/*----------------------------------------------------------------------*/
/* Efuse BT Efuse Section Element Type Definitions (Ref. 4.20)            */
/*----------------------------------------------------------------------*/
#define SDD_EFUSE_BT_BT_TX_PAPOWER_TRIM             0x90
#define SDD_EFUSE_BT_BT_TX_PADRIVER_TRIM            0x91
#define SDD_EFUSE_BT_BT_TX_ISM_TRIM                 0x92
#define SDD_EFUSE_BT_BT_TX_IPM_TRIM                 0x93
#define SDD_EFUSE_BT_ADC_OFFSET                     0x95
#define SDD_EFUSE_BT_ADC_LSB                        0x96
#define SDD_EFUSE_BT_TNOM                           0x99
#define SDD_EFUSE_BT_TOFFSET                        0x9A
#define SDD_EFUSE_BT_BT_PNOM_2441                   0x9C
#define SDD_EFUSE_BT_BT_PNOM_2402                   0x9D
#define SDD_EFUSE_BT_BT_PNOM_2480                   0x9E
#define SDD_EFUSE_BT_SMPS_VSEL                      0xA0
#define SDD_EFUSE_BT_SMPS_BGTR                      0xA1
#define SDD_EFUSE_BT_SMPS_FTR                       0xA2
#define SDD_EFUSE_BT_SMPS_OCP                       0xA3
#define SDD_EFUSE_MAC_ADDRESS                       0xA4
#define SDD_EFUSE_CHIP_ID                           0xA5

/*----------------------------------------------------------------------*/
/*                                          */
/*----------------------------------------------------------------------*/
#define SDD_LAST_SECTION_SIZE (8)

#define SDD_IE_ROUNDING    (4)

#define ROUND_UP_TO_IE(N)    (((N)+SDD_IE_ROUNDING-1) /SDD_IE_ROUNDING *SDD_IE_ROUNDING)

/* Number of Bytes of header priorit to value part in Infromation element */
#define SDD_HDR_SIZE 2

/*----------------------------------------------------------------------*/
/* General Element Type Header                                          */
/*----------------------------------------------------------------------*/
typedef struct SDD_GENERAL_HEADER_ELT_S {
    uint8_t   Type;
    uint8_t   Length;
} SDD_GENERAL_HEADER_ELT;

/*----------------------------------------------------------------------*/
/* SDD Version Element Type (Ref. 5.1.1)                                */
/*----------------------------------------------------------------------*/
typedef struct SDD_VERSION_ELT_S {
    uint8_t   Type;                /* SDD_VERSION_ELT_ID */
    uint8_t   Length;              /* Fixed = 2 */
    uint8_t   SddMajorVersion;
    uint8_t   SddMinorVersion;
} SDD_VERSION_ELT;

/*----------------------------------------------------------------------*/
/* SDD Section Header Element Type (Ref. 5.1.2)                         */
/*----------------------------------------------------------------------*/
typedef struct SDD_SECTION_HEADER_ELT_S {
    uint8_t   Type;                /* SDD_SECTION_HEADER_ELT_ID */
    uint8_t   Length;              /* Fixed = 6 */
    uint8_t   SectionId;
    uint8_t   SectionMajorVersion;
    uint8_t   SectionMinorVersion;
    uint8_t   Reserved;
    uint16_t  SectionLength;
} SDD_SECTION_HEADER_ELT;

/*----------------------------------------------------------------------*/
/* SDD Padding Element Type (Ref. 5.1.3)                                */
/*----------------------------------------------------------------------*/
typedef struct SDD_PADDING_ELT_S {
    uint8_t   Type;                /* SDD_PADDING_ELT_ID */
    uint8_t   Length;              /* Variable */
    uint8_t   Padding[4];
} SDD_PADDING_ELT;

/*----------------------------------------------------------------------*/
/* SDD End of Content Element Type (Ref. 5.1.4)                         */
/*----------------------------------------------------------------------*/
typedef struct SDD_END_OF_CONTENT_ELT_S {
    uint8_t   Type;               /* SDD_END_OF_CONTENT_ELT_D */
    uint8_t   Length;             /* Variable */
    uint8_t   Padding[4];         /* Padding - 0 - 3 bytes to make next element 4 bytes aligned */
} SDD_END_OF_CONTENT_ELT;

#endif // _SDD_DEFS_H_

/*************************************************************************\
                                EOF
\*************************************************************************/

