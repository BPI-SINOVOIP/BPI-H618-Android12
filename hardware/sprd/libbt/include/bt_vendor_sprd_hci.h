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

#ifndef BT_VENDOR_SPRD_HCI_H
#define BT_VENDOR_SPRD_HCI_H

#include "bt_types.h"
#include "bt_vendor_hcidefs.h"
#include "bt_target.h"

#define HCI_EVT_CMD_CMPL_LEN    1
#define HCI_EVT_CMD_CMPL_VSC    3
#define HCI_CMD_PREAMBLE_SIZE   3
#define HCI_CMD_MAX_LEN         258

typedef void (*hci_cback)(void *);

uint8_t sprd_vnd_send_hci_vsc(uint16_t cmd, uint8_t *payload, uint8_t len, hci_cback cback);

#define HCITOOLS_SOCKET 4550

int sprd_vendor_hci_init(void);
void sprd_vendor_hci_cleanup(void);

#define HCI_WRITE_LOCAL_HW_REGISTER 0xFC2D
#define HCI_READ_LOCAL_HW_REGISTER 0xFC2F
#define HCI_WRITE_LOCAL_RF_REGISTER 0xFC30
#define HCI_READ_LOCAL_RF_REGISTER 0xFC31


#endif /*BT_VENDOR_SPRD_HCI_H*/
