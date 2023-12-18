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
 *  Filename:      bt_vendor_brcm_ssp.h
 *
 *  Description:   Contains definitions specific for interfacing with Broadcom
 *                 Bluetooth chipsets for A2DP Offload implementation.
 *
 ******************************************************************************/

#ifndef BT_VENDOR_CONF_H
#define BT_VENDOR_CONF_H

#include "bt_vendor_sprd.h"
#include "bt_target.h"
#include "bt_vendor_sprd.h"
#include "comm.h"

#define UNUSED(x) (void)(x)
#define CONF_ITEM_TABLE(ITEM, ACTION, BUF, LEN) \
  { #ITEM, ACTION, &(BUF.ITEM), LEN, (sizeof(BUF.ITEM) / LEN) }


void vnd_load_configure(const char *p_path, const conf_entry_t *entry);
void set_mac_address(uint8_t *addr);

#endif /*BT_VENDOR_CONF_H*/
