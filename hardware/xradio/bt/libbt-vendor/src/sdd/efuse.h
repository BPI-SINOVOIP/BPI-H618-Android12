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

#ifndef EFUSE_H_
#define EFUSE_H_

#include <stdbool.h>


#define EFUSE_BT_TX_PAPOWER_TRIM_STARTBIT (26)
#define EFUSE_BT_TX_PAPOWER_TRIM_WIDTH (6)

#define EFUSE_BT_TX_PADRIVER_TRIM_STARTBIT (32)
#define EFUSE_BT_TX_PADRIVER_TRIM_WIDTH (6)

#define EFUSE_BT_TX_ISM_TRIM_STARTBIT (38)
#define EFUSE_BT_TX_ISM_TRIM_WIDTH (4)

#define EFUSE_BT_TX_IPM_TRIM_STARTBIT (42)
#define EFUSE_BT_TX_IPM_TRIM_WIDTH (2)

#define EFUSE_ADC_OFFSET_STARTBIT (100)
#define EFUSE_ADC_OFFSET_WIDTH (8)

#define EFUSE_ADC_LSB_STARTBIT (108)
#define EFUSE_ADC_LSB_WIDTH (9)

#define EFUSE_TNOM_STARTBIT (530)
#define EFUSE_TNOM_WIDTH (16)

#define EFUSE_TOFFSET_STARTBIT (546)
#define EFUSE_TOFFSET_WIDTH (12)

#define EFUSE_BT_PNOM_2441_STARTBIT (606)
#define EFUSE_BT_PNOM_2441_WIDTH (16)

#define EFUSE_BT_PNOM_2402_STARTBIT (622)
#define EFUSE_BT_PNOM_2402_WIDTH (16)

#define EFUSE_BT_PNOM_2480_STARTBIT (638)
#define EFUSE_BT_PNOM_2480_WIDTH (16)

#define EFUSE_SMPS_VSEL_STARTBIT (81)
#define EFUSE_SMPS_VSEL_WIDTH (4)

#define EFUSE_SMPS_BGTR_STARTBIT (85)
#define EFUSE_SMPS_BGTR_WIDTH (5)

#define EFUSE_SMPS_FTR_STARTBIT (90)
#define EFUSE_SMPS_FTR_WIDTH (7)

#define EFUSE_SMPS_OCP_STARTBIT (97)
#define EFUSE_SMPS_OCP_WIDTH (3)

#define EFUSE_CHIP_ID_FLAG_STARTBIT (396)
#define EFUSE_CHIP_ID_FLAG_WIDTH (2)

#define EFUSE_CHIP_ID1_STARTBIT (268)
#define EFUSE_CHIP_ID1_WIDTH (128)

#define EFUSE_CHIP_ID2_STARTBIT (398)
#define EFUSE_CHIP_ID2_WIDTH (128)

#define EFUSE_MAC_FLAG_STARTBIT (940)
#define EFUSE_MAC_FLAG_WIDTH (18)

#define EFUSE_MAC_1_STARTBIT (958)
#define EFUSE_MAC_1_WIDTH (48)


typedef enum efuse_field
{
    EFUSE_FIELD_MAC,
    EFUSE_FIELD_CHIPID,
} efuse_field_t;

int efuse_init(void);

void efuse_deinit(void);

bool is_efuse_ready(void);

int efuse_read(uint32_t start_bit, uint32_t bit_num, uint8_t *data);

uint16_t efuse_read_field(efuse_field_t field, uint8_t *data);


#endif /* EFUSE_H_ */
