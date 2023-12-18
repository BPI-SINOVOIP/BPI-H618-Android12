/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
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

/******************************************************************************
 *
 *  Filename:      bt_vendor_sprd_a2dp.h
 *
 *  Description:   Contains definitions specific for interfacing with Broadcom
 *                 Bluetooth chipsets for A2DP Offload implementation.
 *
 ******************************************************************************/

#ifndef BT_VENDOR_SPRD_A2DP_H
#define BT_VENDOR_SPRD_A2DP_H

#include "bt_target.h"
#include "uipc_msg.h"
#include "bt_vendor_sprd_osi.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define UNUSED(x) (void)(x)

#define MULTI_BIT_SET(x) !!(x & (x - 1))

#ifndef SPRD_A2DP_OFFLOAD_MAX_BITPOOL
/* High quality setting @ 44.1 kHz */
#define SPRD_A2DP_OFFLOAD_MAX_BITPOOL 53
#endif

int sprd_vnd_a2dp_execute(bt_vendor_opcode_t, void *ev_data);

#endif /*BT_VENDOR_SPRD_A2DP_H*/
