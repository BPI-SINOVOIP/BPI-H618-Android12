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

#define HCI_GET_CMD_BUF(paramlen)    ((BT_HDR *)GKI_getbuf ((UINT16)(BT_HDR_SIZE + HCIC_PREAMBLE_SIZE + (paramlen))))
#define HCIC_PARAM_SIZE_READ_CMD     0
#define HCIC_PARAM_SIZE_RESET 0
#define HCIC_PARAM_SIZE_SET_EVENT_MASK 8
#define HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG  4
#define HCIC_PARAM_SIZE_WRITE_PARAM1     1
#define HCIC_PARAM_SIZE_WRITE_PARAM3     3

BOOLEAN btsnd_hcic_write_cur_iac_lap (UINT8 num_cur_iac, LAP * const iac_lap)
{
    BT_HDR *p;
    UINT8 *pp;
    int i;

    if ((p = HCI_GET_CMD_BUF(1 + (LAP_LEN * num_cur_iac))) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + 1 + (LAP_LEN * num_cur_iac);
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_WRITE_CURRENT_IAC_LAP);
    UINT8_TO_STREAM  (pp, p->len - HCIC_PREAMBLE_SIZE);

    UINT8_TO_STREAM (pp, num_cur_iac);

    for (i = 0; i < num_cur_iac; i++)
        LAP_TO_STREAM (pp, iac_lap[i]);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_write_pagescan_cfg(UINT16 interval, UINT16 window)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_WRITE_PAGESCAN_CFG);
    UINT8_TO_STREAM  (pp, HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG);

    UINT16_TO_STREAM (pp, interval);
    UINT16_TO_STREAM (pp, window);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_write_scan_enable (UINT8 flag)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_WRITE_PARAM1)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_WRITE_SCAN_ENABLE);
    UINT8_TO_STREAM  (pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM  (pp, flag);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_reset (UINT8 local_controller_id)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_RESET)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_RESET;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_RESET);
    UINT8_TO_STREAM (pp, HCIC_PARAM_SIZE_RESET);

    btu_hcif_send_cmd (local_controller_id,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_set_event_mask(UINT8 local_controller_id, BT_EVENT_MASK event_mask)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_SET_EVENT_MASK)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SET_EVENT_MASK;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_SET_EVENT_MASK);
    UINT8_TO_STREAM  (pp, HCIC_PARAM_SIZE_SET_EVENT_MASK);
    ARRAY8_TO_STREAM (pp, event_mask);

    btu_hcif_send_cmd (local_controller_id,  p);
    return (TRUE);
}


BOOLEAN btsnd_hcic_enable_test_mode (void)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_READ_CMD)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_ENABLE_DEV_UNDER_TEST_MODE);
    UINT8_TO_STREAM  (pp,  HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_set_event_filter (UINT8 filt_type, UINT8 filt_cond_type,
                                     UINT8 *filt_cond, UINT8 filt_cond_len)
{
    BT_HDR *p;
    UINT8 *pp;

    /* Use buffer large enough to hold all sizes in this command */
    if ((p = HCI_GET_CMD_BUF(2 + filt_cond_len)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_SET_EVENT_FILTER);

    if (filt_type)
    {
        p->len = (UINT16)(HCIC_PREAMBLE_SIZE + 2 + filt_cond_len);
        UINT8_TO_STREAM (pp, (UINT8)(2 + filt_cond_len));

        UINT8_TO_STREAM (pp, filt_type);
        UINT8_TO_STREAM (pp, filt_cond_type);

        if (filt_cond_type == HCI_FILTER_COND_DEVICE_CLASS)
        {
            DEVCLASS_TO_STREAM (pp, filt_cond);
            filt_cond += DEV_CLASS_LEN;
            DEVCLASS_TO_STREAM (pp, filt_cond);
            filt_cond += DEV_CLASS_LEN;

            filt_cond_len -= (2 * DEV_CLASS_LEN);
        }
        else if (filt_cond_type == HCI_FILTER_COND_BD_ADDR)
        {
            BDADDR_TO_STREAM (pp, filt_cond);
            filt_cond += BD_ADDR_LEN;

            filt_cond_len -= BD_ADDR_LEN;
        }

        if (filt_cond_len)
            ARRAY_TO_STREAM (pp, filt_cond, filt_cond_len);
    }
    else
    {
        p->len = (UINT16)(HCIC_PREAMBLE_SIZE + 1);
        UINT8_TO_STREAM (pp, 1);

        UINT8_TO_STREAM (pp, filt_type);
    }

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_ble_receiver_test(UINT8 rx_freq)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_WRITE_PARAM1)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_BLE_RECEIVER_TEST);
    UINT8_TO_STREAM  (pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM (pp, rx_freq);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

BOOLEAN btsnd_hcic_ble_transmitter_test(UINT8 tx_freq, UINT8 test_data_len, UINT8 payload)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_WRITE_PARAM3)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM3;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_BLE_TRANSMITTER_TEST);
    UINT8_TO_STREAM  (pp, HCIC_PARAM_SIZE_WRITE_PARAM3);

    UINT8_TO_STREAM (pp, tx_freq);
    UINT8_TO_STREAM (pp, test_data_len);
    UINT8_TO_STREAM (pp, payload);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}

void btsnd_hcic_send_cmd (void *buffer, UINT16 opcode, UINT8 len,
                                 UINT8 *p_data, void *p_cmd_cplt_cback)
{
    BT_HDR *p = (BT_HDR *)buffer;
    UINT8 *pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + len;
    p->offset = sizeof(void *);

    *((void **)pp) = p_cmd_cplt_cback;  /* Store command complete callback in buffer */
    pp += sizeof(void *);               /* Skip over callback pointer */

    UINT16_TO_STREAM (pp, opcode);
    UINT8_TO_STREAM  (pp, len);
    ARRAY_TO_STREAM  (pp, p_data, len);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
}

BOOLEAN btsnd_hcic_read_bd_addr (void)
{
    BT_HDR *p;
    UINT8 *pp;

    if ((p = HCI_GET_CMD_BUF(HCIC_PARAM_SIZE_READ_CMD)) == NULL)
        return (FALSE);

    pp = (UINT8 *)(p + 1);

    p->len    = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM (pp, HCI_READ_BD_ADDR);
    UINT8_TO_STREAM  (pp,  HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd (LOCAL_BR_EDR_CONTROLLER_ID,  p);
    return (TRUE);
}
