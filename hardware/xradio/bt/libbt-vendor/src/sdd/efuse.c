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

#define LOG_TAG "bt_vendor_efuse_sdd"

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <utils/Log.h>
#include <errno.h>

#include "efuse.h"
#include "bt_vendor_xr.h"


#define EFUSE_BIT_NUM    (2048)

#define EFUSE_DIR "/proc/xradio/hwinfo"

#define EFUSE_DBG(fmt, arg...)    ALOGD("<%s:%u>: " fmt "\n", __FUNCTION__, __LINE__, ##arg)
#define EFUSE_ERR(fmt, arg...)    ALOGE("<%s:%u>: " fmt "\n", __FUNCTION__, __LINE__, ##arg)

static uint8_t *efuse_buffer = NULL;


typedef struct efuse_region_info {
    uint16_t flag_start;
    uint16_t flag_bits;        /* MUST less than 32-bit */
    uint16_t data_start;
    uint16_t data_bits;
    uint8_t *buf;            /* temp buffer for write to save read back data */
    uint8_t  buf_len;        /* MUST equal to ((data_bits + 7) / 8) */
} efuse_region_info_t;


int efuse_init(void)
{
    if (efuse_buffer != NULL) {
        ALOGE("already inited");
        return -1;
    }

    efuse_buffer = malloc(EFUSE_BIT_NUM / 8);
    if (efuse_buffer == NULL) {
        EFUSE_ERR("no memory");
        return -1;
    }

    int ret;
    int efuse_fd = open(EFUSE_DIR, O_RDONLY);
    if (efuse_fd <= 0) {
        EFUSE_ERR("open efuse failed(%d):(%d) %s", efuse_fd, errno, strerror(errno));
    }

    if ((ret = read(efuse_fd, efuse_buffer, EFUSE_BIT_NUM / 8)) != (EFUSE_BIT_NUM / 8)) {
        EFUSE_ERR("read efuse but not enough (%d): %s", ret, strerror(errno));
        free(efuse_buffer);
        efuse_buffer = NULL;
        close(efuse_fd);
        return 0;
    }
    EFUSE_DBG("read efuse OK %d", ret);
    close(efuse_fd);
    return 0;
}

void efuse_deinit(void)
{
    if (efuse_buffer == NULL)
        return;
    free(efuse_buffer);
    efuse_buffer = NULL;
}

static void efuse_read_word(uint32_t idx, uint32_t *data)
{
    if (efuse_buffer == NULL) {
        EFUSE_ERR("efuse is not inited");
        return;
    }

    memcpy(data, &efuse_buffer[idx * sizeof(*data)], sizeof(*data));
}

bool is_efuse_ready(void)
{
    uint8_t temp;
    int ret;
    int efuse_fd = open(EFUSE_DIR, O_RDONLY);
    if (efuse_fd <= 0) {
        EFUSE_ERR("efuse file not ready(%d):(%d) %s", efuse_fd, errno, strerror(errno));
        return 0;
    }

    if ((ret = read(efuse_fd, &temp, 1)) != 1) {
        EFUSE_ERR("no efuse data now(%d): %s", ret, strerror(errno));
        close(efuse_fd);
        return 0;
    }
    EFUSE_DBG("efuse file exist!");

    close(efuse_fd);
    return 1;
}

int efuse_read(uint32_t start_bit, uint32_t bit_num, uint8_t *data)
{
    if ((data == NULL)
        || (start_bit >= EFUSE_BIT_NUM)
        || (bit_num == 0)
        || (bit_num > EFUSE_BIT_NUM)
        || (start_bit + bit_num > EFUSE_BIT_NUM)) {
        EFUSE_DBG("start bit %u, bit num %u, data %p", start_bit, bit_num, data);
        return -1;
    }

    uint8_t       *p_data = data;
    uint32_t    bit_shift = start_bit & (32 - 1);
    uint32_t    word_idx = start_bit >> 5;

    uint64_t    buf = 0;
    uint32_t   *efuse_word = (uint32_t *)&buf;
    uint32_t    byte_num = (bit_num + 7) >> 3;
    uint32_t    byte_cnt = byte_num;
    uint32_t    bit_cnt;
    uint32_t    copy_size;

    memset(data, 0, byte_num);

    while (byte_cnt > 0) {
        efuse_read_word((uint8_t)word_idx, &efuse_word[0]);

        if (word_idx + 1 < 64)
            efuse_read_word((uint8_t)word_idx + 1, &efuse_word[1]);
        else
            efuse_word[1] = 0;
        buf = buf >> bit_shift;

        copy_size = (byte_cnt > sizeof(efuse_word[0])) ? sizeof(efuse_word[0]) : byte_cnt;
        memcpy(p_data, &efuse_word[0], copy_size);
        byte_cnt -= copy_size;
        p_data += copy_size;
        word_idx++;
    }

    bit_cnt = bit_num & (8 - 1);
    if (bit_cnt > 0)
        data[byte_num - 1] &= ((1 << bit_cnt) - 1);

    return 0;
}


static int efuse_read_chipid(uint8_t *data)
{
    uint8_t flag = 0;

    /* flag */
    if (efuse_read(EFUSE_CHIP_ID_FLAG_STARTBIT, EFUSE_CHIP_ID_FLAG_WIDTH, &flag) != 0)
        return -1;

    /* chipid */
    if (flag == 0) {
        if (efuse_read(EFUSE_CHIP_ID1_STARTBIT, EFUSE_CHIP_ID1_WIDTH, data) != 0)
            return -1;
    } else {
        if (efuse_read(EFUSE_CHIP_ID2_STARTBIT, EFUSE_CHIP_ID2_WIDTH, data) != 0)
            return -1;
    }

    return 0;
}

static int efuse_read_region(efuse_region_info_t *info, uint8_t *data)
{
#define EFUSE_REGION_ATOMIC_FLAG_MASK    0x3
#define EFUSE_REGION_ATOMIC_FLAG_BITS    2
    uint8_t idx = 0;
    uint32_t flag = 0;
    uint32_t start_bit;

    /* flag */
    if (efuse_read(info->flag_start, info->flag_bits, (uint8_t *)&flag) != 0)
        return -1;
    EFUSE_DBG("r flag 0x%x, start %d, bits %d", flag, info->flag_start, info->flag_bits);

    if (flag == 0) {
        EFUSE_ERR("%s(), flag (%d, %d) is 0", __func__, info->flag_start, info->flag_bits);
        return -1;
    }

    while ((flag & EFUSE_REGION_ATOMIC_FLAG_MASK) == 0) {
        flag = flag >> EFUSE_REGION_ATOMIC_FLAG_BITS;
        idx++;
    }

    /* data */
    start_bit = info->data_start + idx * info->data_bits;
    EFUSE_DBG("r data, start %d, bits %d", start_bit, info->data_bits);
    if (efuse_read(start_bit, info->data_bits, data) != 0)
        return -1;

    return 0;
}

static uint16_t efuse_read_mac(uint8_t *data)
{
    efuse_region_info_t info;

    info.flag_start = EFUSE_MAC_FLAG_STARTBIT;
    info.flag_bits = EFUSE_MAC_FLAG_WIDTH;
    info.data_start = EFUSE_MAC_1_STARTBIT;
    info.data_bits = EFUSE_MAC_1_WIDTH;
    info.buf = NULL;
    info.buf_len = 0;
    return efuse_read_region(&info, data);
}

uint16_t efuse_read_field(efuse_field_t field, uint8_t *data)
{
    switch (field) {
        case EFUSE_FIELD_MAC:
            return efuse_read_mac(data);
        case EFUSE_FIELD_CHIPID:
            return efuse_read_chipid(data);
        default:
            EFUSE_ERR("%s(), %d, read field %d", __func__, __LINE__, field);
            return -1;
    }
}
