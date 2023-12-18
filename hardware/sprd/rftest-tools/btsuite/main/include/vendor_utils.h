/******************************************************************************
 *
 *  Copyright (C) 2017 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#ifndef BT_LIBBT_SPRD_VENDOR_UTILS_H_
#define BT_LIBBT_SPRD_VENDOR_UTILS_H_

#include <string.h>
#include <log/log.h>
#include "bt_types.h"

#ifndef UNUSED
#define UNUSED(expr) do { (void)(expr); } while (0)
#endif

#ifndef LOG_TAG
#define LOG_TAG "libbt-sprd_suite"
#endif

#ifndef UNUSED_ATTR
#define UNUSED_ATTR __attribute__((unused))
#endif

#ifdef SPRD_SUITE_TEST
#define BTD(format, ...) {fprintf (stdout, format"\n", ## __VA_ARGS__);}
#define BTW(format, ...) {fprintf (stdout, format"\n", ## __VA_ARGS__);}
#define BTE(format, ...) {fprintf (stdout, format"\n", ## __VA_ARGS__);}
#else
#define BTD(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#define BTW(param, ...) {ALOGW(param, ## __VA_ARGS__);}
#define BTE(param, ...) {ALOGE(param, ## __VA_ARGS__);}
#endif

typedef enum {
  DATA_TYPE_COMMAND = 1,
  DATA_TYPE_ACL     = 2,
  DATA_TYPE_SCO     = 3,
  DATA_TYPE_EVENT   = 4
} serial_data_type_t;

// 2 bytes for opcode, 1 byte for parameter length (Volume 2, Part E, 5.4.1)
#define HCI_COMMAND_PREAMBLE_SIZE 3
// 2 bytes for handle, 2 bytes for data length (Volume 2, Part E, 5.4.2)
#define HCI_ACL_PREAMBLE_SIZE 4
// 2 bytes for handle, 1 byte for data length (Volume 2, Part E, 5.4.3)
#define HCI_SCO_PREAMBLE_SIZE 3
// 1 byte for event code, 1 byte for parameter length (Volume 2, Part E, 5.4.4)
#define HCI_EVENT_PREAMBLE_SIZE 2

typedef void (*data_ready_cb)(serial_data_type_t type);

typedef struct {
    data_ready_cb data_ready;
} hci_h4_callbacks_t;

typedef void (*command_complete_cb)(BT_HDR *response, void *context);
typedef void (*command_status_cb)(uint8_t status, BT_HDR *command, void *context);

typedef struct
{
    UINT16  opcode;
    UINT16  param_len;
    UINT8   *p_param_buf;
} tBTM_VSC_CMPL;

typedef void (tBTM_CMPL_CB) (void *p1);

#endif  // BT_LIBBT_SPRD_VENDOR_UTILS_H_
