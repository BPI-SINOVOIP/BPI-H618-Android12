/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#include <assert.h>
#include <cutils/properties.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

#include "bt_types.h"

#include "vendor_h4.h"
#include "vendor_hci.h"
#include "vendor_hcif.h"
#include "vendor_utils.h"
#include "vendor_hcicmds.h"
#include "vendor_suite.h"
#include "hcidefs.h"

#include "buffer_allocator.h"


#include "osi/include/eager_reader.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/reactor.h"
#include "osi/include/thread.h"
#include "osi/include/future.h"
#include "osi/include/list.h"
#include "osi/include/fixed_queue.h"

int bt_test_mode_enable(void)
{
    uint8_t cond = HCI_DO_AUTO_ACCEPT_CONNECT;
    uint8_t scan_mode = HCI_PAGE_SCAN_ENABLED;
    uint8_t general_inq_lap[3] = {0x9e,0x8b,0x33};

    BTD("BTM: BTM_EnableTestMode");

    if (!btsnd_hcic_set_event_filter(HCI_FILTER_CONNECTION_SETUP,
                                     HCI_FILTER_COND_NEW_DEVICE,
                                     &cond, sizeof(cond)))
        return -1;

    if (!btsnd_hcic_write_scan_enable(scan_mode))
        return -1;

    if (!btsnd_hcic_write_cur_iac_lap (1, (LAP * const) &general_inq_lap))
        return -1;

    scan_mode |= HCI_INQUIRY_SCAN_ENABLED;
    if (!btsnd_hcic_write_scan_enable(scan_mode))
        return -1;

    if (!btsnd_hcic_set_event_mask(LOCAL_BR_EDR_CONTROLLER_ID,
    (UINT8 *)"\x00\x00\x00\x00\x00\x00\x00\x00"))
        return -1;

    if (!btsnd_hcic_enable_test_mode ())
        return -1;

    return 0;
}

void bt_test_mode_disable(void)
{
    btsnd_hcic_reset(LOCAL_BR_EDR_CONTROLLER_ID);
}

void BTM_SendHciCommand(UINT16 opcode, UINT8 param_len,
                                      UINT8 *p_param_buf, hci_cmd_complete_cb *p_cb)
{
    void *p_buf;
    if ((p_buf = GKI_getbuf((UINT16)(sizeof(BT_HDR) + sizeof (tBTM_CMPL_CB *) +
                            param_len + HCIC_PREAMBLE_SIZE))) != NULL)
    {
        /* Send the HCI command (opcode will be OR'd with HCI_GRP_VENDOR_SPECIFIC) */
        btsnd_hcic_send_cmd (p_buf, opcode, param_len, p_param_buf, (void *)p_cb);
    }
}
