/******************************************************************************
 *
 *  Copyright (C) 2019-2021 Aicsemi Corporation
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

#define LOG_TAG "bt_hwcfg_usb"
#define AICBT_RELEASE_NAME "20220429_BT_ANDROID_1x.0"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_aic.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"
#include <unistd.h>
#include <endian.h>
#include <byteswap.h>
#include <unistd.h>

#include "bt_vendor_lib.h"
#include "hardware.h"
#include "aic_common.h"

void hw_usb_config_cback(void *p_evt_buf);

/******************************************************************************
**  Static variables
******************************************************************************/

/*******************************************************************************
**
** Function         hw_usb_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_usb_config_cback(void *p_mem)
{
    HC_BT_HDR   *p_evt_buf = NULL;
    uint8_t     *p = NULL;//, *pp=NULL;
    uint8_t     status = 0;
    uint16_t    opcode = 0;
    HC_BT_HDR   *p_buf = NULL;
    uint8_t     is_proceeding = FALSE;

    if (p_mem != NULL) {
        p_evt_buf = (HC_BT_HDR *) p_mem;
        status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET);
        p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE_OFFSET;
        STREAM_TO_UINT16(opcode,p);
    }

    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    /*a cut fc6d status==1*/
    if (((status == 0) ||(opcode == HCI_VSC_READ_ROM_VERSION)) && bt_vendor_cbacks)
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_MAX_LEN);

    if (p_buf != NULL) {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        BTVNDDBG("hw_cfg_cb.state = %d", hw_cfg_cb.state);
        switch (hw_cfg_cb.state) {
            case HW_CFG_RESET_CHANNEL_CONTROLLER:
                is_proceeding = hw_config_set_bdaddr(p_buf);
                break;
            case HW_CFG_SET_BD_ADDR:
                if(bt_rf_need_config == true)
                {
                    is_proceeding = hw_wr_rf_mdm_regs(p_buf);
                    break;
                }
                else
                {
                    if(AIC_RF_MODE_BTWIFI_COMBO == hw_get_bt_rf_mode())
                    {
                        is_proceeding = hw_aic_bt_pta_en(p_buf);
                        break;
                    }
                }
                ALOGI("vendor lib fwcfg completed");
                bt_vendor_cbacks->dealloc(p_buf);
                bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                hw_cfg_cb.state = 0;

                is_proceeding = TRUE;

                break;
            case HW_CFG_WR_RF_MDM_REGS:
                #if 1
                is_proceeding = hw_wr_rf_mdm_regs(p_buf);
                ALOGE("AICHW_CFG %x\n",HW_CFG_WR_RF_MDM_REGS);
                #endif
                break;
            case HW_CFG_WR_RF_MDM_REGS_END:
                is_proceeding = hw_set_rf_mode(p_buf);
                break;
            case HW_CFG_SET_RF_MODE:
                if(bt_rf_need_calib == true)
                {
                    ALOGE("AICHW_CFG %x\n",HW_CFG_SET_RF_MODE);
                    is_proceeding = hw_rf_calib_req(p_buf);
                    break;
                }
                ///no break if no need to do rf calib
            case HW_CFG_RF_CALIB_REQ:
                ALOGE("AICHW_CFG %x\n",HW_CFG_RF_CALIB_REQ);
                if(AIC_RF_MODE_BTWIFI_COMBO == hw_get_bt_rf_mode())
                {
                    is_proceeding = hw_aic_bt_pta_en(p_buf);
                    break;
                }
            case HW_CFG_UPDATE_CONFIG_INFO:
                ALOGI("vendor lib fwcfg completed");
                bt_vendor_cbacks->dealloc(p_buf);
                bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                hw_cfg_cb.state = 0;

                is_proceeding = TRUE;
                break;

            default:
                break;
        }
    }

    /* Free the RX event buffer */
    if ((bt_vendor_cbacks) && (p_evt_buf != NULL))
        bt_vendor_cbacks->dealloc(p_evt_buf);

    if (is_proceeding == FALSE) {
        ALOGE("vendor lib fwcfg aborted!!!");
        if (bt_vendor_cbacks) {
            if (p_buf != NULL)
                bt_vendor_cbacks->dealloc(p_buf);

            int lmp_sub_current;
            userial_vendor_usb_ioctl(DWFW_CMPLT, &lmp_sub_current);
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }

        hw_cfg_cb.state = 0;
    }
}

/*******************************************************************************
**
** Function        hw__usb_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_usb_config_start(char transtype, uint32_t usb_id)
{
    AIC_UNUSED(transtype);
    uint16_t usb_pid = usb_id & 0x0000ffff;
    uint16_t usb_vid = (usb_id >> 16) & 0x0000ffff;
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    BTVNDDBG("AICBT_RELEASE_NAME: %s", AICBT_RELEASE_NAME);
    BTVNDDBG("\nAicsemi libbt-vendor_usb Version %s \n", AIC_VERSION);
    BTVNDDBG("hw_usb_config_start, transtype = 0x%x, pid = 0x%04x, vid = 0x%04x \n", transtype, usb_pid, usb_vid);

    if (bt_vendor_cbacks) {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE);
        if (p_buf) {
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            p = (uint8_t *) (p_buf + 1);

            p = (uint8_t *)(p_buf + 1);
            UINT16_TO_STREAM(p, HCI_VENDOR_RESET);
            *p++ = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            hw_cfg_cb.state = HW_CFG_RESET_CHANNEL_CONTROLLER;
            bt_vendor_cbacks->xmit_cb(HCI_VENDOR_RESET, p_buf, hw_usb_config_cback);
        } else {
            ALOGE("%s buffer alloc fail!", __func__);
        }
    } else {
        ALOGE("%s call back is null", __func__);
    }
}

