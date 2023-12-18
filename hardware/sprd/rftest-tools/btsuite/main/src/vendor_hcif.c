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
#include "hcidefs.h"
#include "vendor_suite.h"

#include "buffer_allocator.h"


#include "osi/include/eager_reader.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/reactor.h"
#include "osi/include/thread.h"
#include "osi/include/future.h"
#include "osi/include/list.h"
#include "osi/include/fixed_queue.h"

fixed_queue_t *btu_hci_msg_queue;
thread_t *bt_workqueue_thread;
static const char *BT_WORKQUEUE_NAME = "bt_workqueue";
extern bdremote_t *bdrmt_suite_dev;
extern bool device_sem_post;
extern void read_local_address_cb(BD_ADDR address);
extern void read_discovery_device_cb(void);

extern void transmit_command(
    BT_HDR *command,
    command_complete_cb complete_callback,
    command_status_cb status_callback,
    void *context);

void btm_vendor_specific_evt (UINT8 *p, UINT8 evt_len)
{
    UNUSED(p);
    UNUSED(evt_len);
}

void btm_vendor_ble_adv_process (UINT8 *p, UINT8 evt_len)
{
//    UINT8 loop_tmp = 0;
//    for (; loop_tmp < evt_len; loop_tmp++)
//        BTD("btm_vendor_ble_adv_process data[%d] = 0x%02X", loop_tmp, p[loop_tmp]);
    if (evt_len < 12)
        return;
    UINT8 loop_i = 0;
//    char rssi;
    (*bdrmt_suite_dev).rssi_val = p[evt_len - 1];
    UINT8 le_sub_evt;
    STREAM_TO_UINT8(le_sub_evt, p);
    UINT8 num_of_rep;
    STREAM_TO_UINT8(num_of_rep, p);
    UINT8 rep_evt_type;
    STREAM_TO_UINT8(rep_evt_type, p);
    UINT8 address_type;
    STREAM_TO_UINT8(address_type, p);
    UINT8 address[6];
    for(; loop_i < 6; loop_i++) {
        STREAM_TO_UINT8(address[5-loop_i], p);
    }
    memcpy((*bdrmt_suite_dev).addr_u8, address, 6);
    UINT8 adv_data_len;
    STREAM_TO_UINT8(adv_data_len, p);
    UINT8 name_length;
    UINT8 name_type;
    UINT8 name[MAX_REMOTE_DEVICE_NAME_LEN] = {0};
    STREAM_TO_UINT8(name_length, p);
    if (name_length != 0) {
        if (name_length >= MAX_REMOTE_DEVICE_NAME_LEN)
            name_length = MAX_REMOTE_DEVICE_NAME_LEN;
        STREAM_TO_UINT8(name_type, p);
        if (name_type == 0x09) {
            for(loop_i = 0; loop_i < name_length - 1; loop_i++) {
                STREAM_TO_UINT8(name[loop_i], p);
            }
            memcpy((*bdrmt_suite_dev).name, name, name_length - 1);
//            for (loop_tmp = 0; loop_tmp < name_length - 1; loop_tmp++)
//                BTD("btm_vendor_ble_adv_process name_data[%d] = 0x%02X", loop_tmp, (*bdrmt_suite_dev).name[loop_tmp]);
        }
    }
//    BTD("btm_vendor_ble_adv_process rssi = 0x%02X", (*bdrmt_suite_dev).rssi_val);
    if(!device_sem_post) {
        device_sem_post = true;
        read_discovery_device_cb();
    }
}

void btm_vendor_ext_inq_process (UINT8 *p, UINT8 evt_len)
{
//    UINT8 loop_tmp = 0;
//    for (loop_tmp = 0; loop_tmp < evt_len; loop_tmp++)
//        BTD("btm_vendor_ext_inq_process data[%d] = 0x%02X", loop_tmp, p[loop_tmp]);
    if (evt_len < 13)
        return;
    UINT8 loop_i = 0;
    UINT8 num_res;
    STREAM_TO_UINT8(num_res, p);
    UINT8 address[6];
    for(loop_i = 0; loop_i < 6; loop_i++) {
        STREAM_TO_UINT8(address[5-loop_i], p);
    }
    memcpy((*bdrmt_suite_dev).addr_u8, address, 6);
    UINT8 page_scan_mode;
    STREAM_TO_UINT8(page_scan_mode, p);
    UINT8 resver;
    STREAM_TO_UINT8(resver, p);
    UINT8 cla_of_dev[3];
    for(loop_i = 0; loop_i < 3; loop_i++) {
        STREAM_TO_UINT8(cla_of_dev[2-loop_i], p);
    }
    UINT8 clock_off[2];
    for(loop_i = 0; loop_i < 2; loop_i++) {
        STREAM_TO_UINT8(clock_off[1-loop_i], p);
    }
    char rssi;
    STREAM_TO_UINT8(rssi, p);
    (*bdrmt_suite_dev).rssi_val = rssi;
    UINT8 name_length;
    UINT8 name_type;
    UINT8 name[MAX_REMOTE_DEVICE_NAME_LEN] = {0};
    STREAM_TO_UINT8(name_length, p);
    if (name_length != 0) {
        if (name_length >= MAX_REMOTE_DEVICE_NAME_LEN)
            name_length = MAX_REMOTE_DEVICE_NAME_LEN;
        STREAM_TO_UINT8(name_type, p);
        if (name_type == 0x09) {
            for(loop_i = 0; loop_i < name_length - 1; loop_i++) {
                STREAM_TO_UINT8(name[loop_i], p);
            }
            memcpy((*bdrmt_suite_dev).name, name, name_length - 1);
//            for (loop_tmp = 0; loop_tmp < name_length - 1; loop_tmp++)
//                BTD("btm_vendor_ext_inq_process name_data[%d] = 0x%02X", loop_tmp, (*bdrmt_suite_dev).name[loop_tmp]);
        }
    }
//    BTD("btm_vendor_ext_inq_process rssi = 0x%02X", rssi);
    if(!device_sem_post) {
        device_sem_post = true;
        read_discovery_device_cb();
    }
}

void btm_vsc_complete (UINT8 *p, UINT16 opcode, UINT16 evt_len,
                       tBTM_CMPL_CB *p_vsc_cplt_cback)
{
    tBTM_VSC_CMPL   vcs_cplt_params;

    /* If there was a callback address for vcs complete, call it */
    if (p_vsc_cplt_cback)
    {
        /* Pass paramters to the callback function */
        vcs_cplt_params.opcode = opcode;        /* Number of bytes in return info */
        vcs_cplt_params.param_len = evt_len;    /* Number of bytes in return info */
        vcs_cplt_params.p_param_buf = p;
        (*p_vsc_cplt_cback)(&vcs_cplt_params);  /* Call the VSC complete callback function */
    }
}

void btm_read_local_addr_complete (UINT8 *p)
{
    UINT8           status;
    BD_ADDR local_addr;

    STREAM_TO_UINT8  (status, p);

    if (status == HCI_SUCCESS)
    {
        STREAM_TO_BDADDR (local_addr, p);
        read_local_address_cb(local_addr);
    }
}


void btu_hcif_process_event (UNUSED_ATTR UINT8 controller_id, BT_HDR *p_msg)
{
    UINT8   *p = (UINT8 *)(p_msg + 1) + p_msg->offset;
    UINT8   hci_evt_code, hci_evt_len;
    STREAM_TO_UINT8  (hci_evt_code, p);
    STREAM_TO_UINT8  (hci_evt_len, p);

//    BTD("process event: 0x%04X", hci_evt_code);

    switch (hci_evt_code)
    {
        case HCI_VENDOR_SPECIFIC_EVT:
                btm_vendor_specific_evt (p, hci_evt_len);
            break;
        case HCI_BLE_EVENT:
                btm_vendor_ble_adv_process (p, hci_evt_len);
            break;
        case HCI_EXTENDED_INQUIRY_RESULT_EVT:
                btm_vendor_ext_inq_process (p, hci_evt_len);
        default:
            break;
    }
}

static void btu_hcif_hdl_command_status (UINT16 opcode, UINT8 status, UINT8 *p_cmd,
                                         void *p_vsc_status_cback)
{
    BTD("return cmd status: 0x%04X", opcode);

    UNUSED(p_cmd);
    switch (opcode)
    {
        case HCI_EXIT_SNIFF_MODE:
            break;

        default:
            /* If command failed to start, we may need to tell BTM */
            if (status != HCI_SUCCESS)
            {
                switch (opcode)
                {
                    case HCI_READ_RMT_EXT_FEATURES:
                        break;

                    default:
                        if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC)
                            btm_vsc_complete (&status, opcode, 1, (tBTM_CMPL_CB *)p_vsc_status_cback);
                        break;
                }

            }
            else
            {
                if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC)
                    btm_vsc_complete (&status, opcode, 1, (tBTM_CMPL_CB *)p_vsc_status_cback);
            }
    }
}

static void btu_hcif_hdl_command_complete (UINT16 opcode, UINT8 *p, UINT16 evt_len,
                                           void *p_cplt_cback)
{
    switch (opcode)
    {
        case HCI_READ_LOCAL_NAME:
            break;

        case HCI_READ_BD_ADDR:
            btm_read_local_addr_complete (p);
            break;

        default:
            if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC)
                btm_vsc_complete (p, opcode, evt_len, (tBTM_CMPL_CB *)p_cplt_cback);
            break;
    }
}
static void btu_hcif_command_status_evt_on_task(BT_HDR *event)
{
    command_status_hack_t *hack = (command_status_hack_t *)&event->data[0];

    command_opcode_t opcode;
    uint8_t *stream = hack->command->data + hack->command->offset;
    STREAM_TO_UINT16(opcode, stream);

    btu_hcif_hdl_command_status(
      opcode,
      hack->status,
      stream,
      hack->context);

    osi_free(hack->command);
    osi_free(event);
}

static void btu_hcif_command_status_evt(uint8_t status, BT_HDR *command, void *context)
{
    BT_HDR *event = osi_calloc(sizeof(BT_HDR) + sizeof(command_status_hack_t));
    command_status_hack_t *hack = (command_status_hack_t *)&event->data[0];

    hack->callback = btu_hcif_command_status_evt_on_task;
    hack->status = status;
    hack->command = command;
    hack->context = context;

    event->event = BTU_POST_TO_TASK_NO_GOOD_HORRIBLE_HACK;

    fixed_queue_enqueue(btu_hci_msg_queue, event);
}

static void btu_hcif_command_complete_evt_on_task(BT_HDR *event)
{
    command_complete_hack_t *hack = (command_complete_hack_t *)&event->data[0];

    command_opcode_t opcode;
    uint8_t *stream = hack->response->data + hack->response->offset + 3; // 2 to skip the event headers, 1 to skip the command credits
    STREAM_TO_UINT16(opcode, stream);

    btu_hcif_hdl_command_complete(
      opcode,
      stream,
      hack->response->len - 5, // 3 for the command complete headers, 2 for the event headers
      hack->context);

    osi_free(hack->response);
    osi_free(event);
}

static void btu_hcif_command_complete_evt(BT_HDR *response, void *context)
{
    BT_HDR *event = osi_calloc(sizeof(BT_HDR) + sizeof(command_complete_hack_t));
    command_complete_hack_t *hack = (command_complete_hack_t *)&event->data[0];

    hack->callback = btu_hcif_command_complete_evt_on_task;
    hack->response = response;
    hack->context = context;

    event->event = BTU_POST_TO_TASK_NO_GOOD_HORRIBLE_HACK;

    fixed_queue_enqueue(btu_hci_msg_queue, event);
}

void btu_hcif_send_cmd (UNUSED_ATTR UINT8 controller_id, BT_HDR *p_buf)
{
    if (!p_buf)
      return;

    uint16_t opcode;
    uint8_t *stream = p_buf->data + p_buf->offset;
    void * vsc_callback = NULL;

    STREAM_TO_UINT16(opcode, stream);

    // Eww...horrible hackery here
    /* If command was a VSC, then extract command_complete callback */
    if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC
        || (opcode == HCI_BLE_RAND)
        || (opcode == HCI_BLE_ENCRYPT)
       ) {
        vsc_callback = *((void **)(p_buf + 1));
    }

    transmit_command(
      p_buf,
      btu_hcif_command_complete_evt,
      btu_hcif_command_status_evt,
      vsc_callback);

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    btu_check_bt_sleep ();
#endif
}

static void btu_hci_msg_process(BT_HDR *p_msg) {
    /* Determine the input message type. */
    switch (p_msg->event & BT_EVT_MASK)
    {
        case BTU_POST_TO_TASK_NO_GOOD_HORRIBLE_HACK: // TODO(zachoverflow): remove this
            ((post_to_task_hack_t *)(&p_msg->data[0]))->callback(p_msg);
            break;
        case BT_EVT_TO_BTU_HCI_EVT:
            btu_hcif_process_event ((UINT8)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
            GKI_freebuf(p_msg);
            break;

        case BT_EVT_TO_BTU_HCI_CMD:
            btu_hcif_send_cmd ((UINT8)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
            break;

        default:;
                GKI_freebuf (p_msg);

            break;
    }

}

void btu_hci_msg_ready(fixed_queue_t *queue, UNUSED_ATTR void *context) {
    BT_HDR *p_msg = (BT_HDR *)fixed_queue_dequeue(queue);
    btu_hci_msg_process(p_msg);
}

void hcif_start_up() {

    bt_workqueue_thread = thread_new(BT_WORKQUEUE_NAME);
    if (bt_workqueue_thread == NULL)
        goto error_exit;

    btu_hci_msg_queue = fixed_queue_new(SIZE_MAX);
    if (btu_hci_msg_queue == NULL) {
      LOG_ERROR("%s unable to allocate hci message queue.", __func__);
      return;
    }


  fixed_queue_register_dequeue(btu_hci_msg_queue,
      thread_get_reactor(bt_workqueue_thread),
      btu_hci_msg_ready,
      NULL);

  error_exit:;

}

void hcif_shut_down() {
  fixed_queue_unregister_dequeue(btu_hci_msg_queue);
}
