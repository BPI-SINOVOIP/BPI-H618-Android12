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
 *  This is the stream state machine for the BRCM offloaded advanced audio.
 *
 ******************************************************************************/

#define LOG_TAG "bt_vnd_ssp"

#include <string.h>
#include <pthread.h>
#include <log/log.h>
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
#include "bt_vendor_sprd_hci.h"
#include "bt_vendor_sprd_ssp.h"

#include "algo_api.h"
#include "algo_utils.h"

#define BDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}

/*****************************************************************************
** Constants and types
*****************************************************************************/

#define PRIVATEK_P256_KEY "3f49f6d4a3c55f3874c9b3e3d2103f504aff607beb40b7995899b8a6cd3c1abd"
#define PRIVATEK_P192_KEY "07915f86918ddc27005df1d6cf0c142b625ed2eff4a518ff"

#define SSP_ENABLE_IN_HOST 0x01
#define SSP_ENABLE_IN_CONTROLLER 0x00


#define HCI_SET_SUPER_SSP_ENABLE 0xFCB0
#define HCI_SET_OWN_PP192_KEY 0xFCB1
#define HCI_SET_OWN_PP256_KEY 0xFCB2

#define HCI_SET_P192_DHKEY 0xFCB3
#define HCI_SET_P256_DHKEY 0xFCB4

#define HCI_PEER_P192_PUBLIC_NOTIFICATION_EVT 0xB0
#define HCI_PEER_P256_PUBLIC_NOTIFICATION_EVT 0xB1
#define HCI_SUPER_SSP_REFRESH_REQUEST_EVT 0xB2


uint8_t p192_private_key[24] = {0};
uint8_t p192_public_key_x[24] = {0};
uint8_t p192_public_key_y[24] = {0};

uint8_t p256_private_key[32] = {0};
uint8_t p256_public_key_x[32] = {0};
uint8_t p256_public_key_y[32] = {0};

/*******************************************************************************
** Local Utility Functions
*******************************************************************************/
static uint8_t sprd_vnd_set_own_pp256_key(void);
static void sprd_vend_ssp_key_init(void);

static void log_bin_to_hexstr(uint8_t *bin, uint8_t binsz, const char *log_tag)
{
  char     *str, hex_str[]= "0123456789abcdef";
  uint8_t  i;

  str = (char *)malloc(binsz * 3);
  if (!str) {
    BDBG("%s alloc failed", __FUNCTION__);
    return;
  }

  for (i = 0; i < binsz; i++) {
      str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
      str[(i * 3) + 1] = hex_str[(bin[i]     ) & 0x0F];
      str[(i * 3) + 2] = ' ';
  }
  str[(binsz * 3) - 1] = 0x00;
  BDBG("%s %s", log_tag, str);
  free(str);
}


void sprd_vnd_hci_ssp_cback(void *pmem)
{
    HC_BT_HDR    *p_evt_buf = (HC_BT_HDR *)pmem;
    uint8_t     *p, len, vsc_result, uipc_opcode;
    uint16_t    vsc_opcode, uipc_event;
    HC_BT_HDR    *p_buf = NULL;
    bt_vendor_op_result_t status = BT_VND_OP_RESULT_SUCCESS;

    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_LEN;
    len = *p;
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_VSC;
    STREAM_TO_UINT16(vsc_opcode,p);
    vsc_result = *p++;

    log_bin_to_hexstr(((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_VSC), len-1, __FUNCTION__);

    if (vsc_result != 0) {
        ALOGE("%s Failed VSC Op %04x", __FUNCTION__, vsc_opcode);
        status = BT_VND_OP_RESULT_FAIL;
    }
    else if (vsc_opcode == HCI_SET_SUPER_SSP_ENABLE) {
        sprd_vend_ssp_key_init();
    }
    else if (vsc_opcode == HCI_SET_OWN_PP192_KEY) {
        sprd_vnd_set_own_pp256_key();
    }
    else if (vsc_opcode == HCI_SET_OWN_PP256_KEY) {
        bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
    }

    else if (vsc_opcode == HCI_SET_P192_DHKEY) {

    }
    else if (vsc_opcode == HCI_SET_P256_DHKEY) {

    }

    /* Free the RX event buffer */
    bt_vendor_cbacks->dealloc(p_evt_buf);
}

static uint8_t sprd_vnd_set_super_ssp_enable(uint8_t enable)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;

    BDBG("%s, %s", __FUNCTION__,
        enable ? "enable" : "disable");
    p = msg_req;
    UINT8_TO_STREAM(p, enable ? SSP_ENABLE_IN_HOST : SSP_ENABLE_IN_CONTROLLER);
    UINT8_TO_STREAM(p, enable ? SSP_ENABLE_IN_HOST : SSP_ENABLE_IN_CONTROLLER);

    sprd_vnd_send_hci_vsc(HCI_SET_SUPER_SSP_ENABLE, msg_req, (uint8_t)(p - msg_req), sprd_vnd_hci_ssp_cback);
    return 0;
}

static uint8_t sprd_vnd_set_own_pp192_key(void)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;

    BDBG("%s", __FUNCTION__);
    p = msg_req;

    algo_p192_generate_private_key(p192_private_key);
    //char2bytes(PRIVATEK_P192_KEY, p192_private_key, sizeof(p192_private_key));
    dump_hex("p192_private_key", p192_private_key, sizeof(p192_private_key));

    algo_p192_generate_public_key(p192_private_key, p192_public_key_x, p192_public_key_y);
    dump_hex("p192_public_key_x", p192_public_key_x, sizeof(p192_public_key_x));
    dump_hex("p192_public_key_y", p192_public_key_y, sizeof(p192_public_key_y));

    memcpy(p, p192_public_key_x, sizeof(p192_public_key_x));
    p += sizeof(p192_public_key_x);

    memcpy(p, p192_public_key_y, sizeof(p192_public_key_y));
    p += sizeof(p192_public_key_y);

    memcpy(p, p192_private_key, sizeof(p192_private_key));
    p += sizeof(p192_private_key);

    sprd_vnd_send_hci_vsc(HCI_SET_OWN_PP192_KEY, msg_req, (uint8_t)(p - msg_req), sprd_vnd_hci_ssp_cback);
    return 0;
}


static uint8_t sprd_vnd_set_own_pp256_key(void)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;

    BDBG("%s", __FUNCTION__);
    p = msg_req;

    algo_p256_generate_private_key(p256_private_key);
    //char2bytes(PRIVATEK_P256_KEY, p256_private_key, sizeof(p256_private_key));
    dump_hex("p256_private_key", p256_private_key, sizeof(p256_private_key));

    algo_p256_generate_public_key(p256_private_key, p256_public_key_x, p256_public_key_y);
    dump_hex("p256_public_key_x", p256_public_key_x, sizeof(p256_public_key_x));
    dump_hex("p256_public_key_y", p256_public_key_y, sizeof(p256_public_key_y));

    memcpy(p, p256_public_key_x, sizeof(p256_public_key_x));
    p += sizeof(p256_public_key_x);

    memcpy(p, p256_public_key_y, sizeof(p256_public_key_y));
    p += sizeof(p256_public_key_y);

    memcpy(p, p256_private_key, sizeof(p256_private_key));
    p += sizeof(p256_private_key);

    sprd_vnd_send_hci_vsc(HCI_SET_OWN_PP256_KEY, msg_req, (uint8_t)(p - msg_req), sprd_vnd_hci_ssp_cback);
    return 0;
}


static uint8_t sprd_vnd_set_p192_dhkey(uint8_t *address, uint8_t *peer_pk_x, uint8_t *peer_pk_y)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;
    uint8_t dhkey[24];

    BDBG("%s", __FUNCTION__);
    p = msg_req;

    dump_hex("peer_pk_x", peer_pk_x, 24);
    dump_hex("peer_pk_y", peer_pk_y, 24);
    algo_p192_generate_dhkey(p192_private_key, peer_pk_x, peer_pk_y, dhkey);
    dump_hex("dhkey", dhkey, sizeof(dhkey));

    memcpy(p, address, 6);
    p += 6;
    memcpy(p, dhkey, sizeof(dhkey));
    p += sizeof(dhkey);

    sprd_vnd_send_hci_vsc(HCI_SET_P192_DHKEY, msg_req, (uint8_t)(p - msg_req), sprd_vnd_hci_ssp_cback);
    return 0;
}

static uint8_t sprd_vnd_set_p256_dhkey(uint8_t *address, uint8_t *peer_pk_x, uint8_t *peer_pk_y)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;
    uint8_t dhkey[32];

    BDBG("%s", __FUNCTION__);
    p = msg_req;

    dump_hex("peer_pk_x", peer_pk_x, 32);
    dump_hex("peer_pk_y", peer_pk_y, 32);
    algo_p256_generate_dhkey(p256_private_key, peer_pk_x, peer_pk_y, dhkey);
    dump_hex("dhkey", dhkey, sizeof(dhkey));

    memcpy(p, address, 6);
    p += 6;
    memcpy(p, dhkey, sizeof(dhkey));
    p += sizeof(dhkey);

    sprd_vnd_send_hci_vsc(HCI_SET_P256_DHKEY, msg_req, (uint8_t)(p - msg_req), sprd_vnd_hci_ssp_cback);
    return 0;
}

static void sprd_vend_ssp_key_init(void)
{
    sprd_vnd_set_own_pp192_key();
}

void sprd_vse_cback(uint8_t len, uint8_t *p)
{
    uint8_t   sub_event;

    /* Check if this is a BLE RSSI vendor specific event */
    STREAM_TO_UINT8(sub_event, p);
    len--;

    BDBG("sprd_vse_cback: 0x%02x", sub_event);
    if (sub_event == HCI_PEER_P192_PUBLIC_NOTIFICATION_EVT) {
        uint8_t peer_public_x[24], peer_public_y[24], address[6];

        memcpy(address, p, sizeof(address));
        p += sizeof(address);
        memcpy(peer_public_x, p, sizeof(peer_public_x));
        p += sizeof(peer_public_x);
        memcpy(peer_public_y, p, sizeof(peer_public_y));
        p += sizeof(peer_public_y);
        sprd_vnd_set_p192_dhkey(address, peer_public_x, peer_public_y);

    } else if (sub_event == HCI_PEER_P256_PUBLIC_NOTIFICATION_EVT) {

        uint8_t peer_public_x[32], peer_public_y[32], address[6];

        memcpy(address, p, sizeof(address));
        p += sizeof(address);
        memcpy(peer_public_x, p, sizeof(peer_public_x));
        p += sizeof(peer_public_x);
        memcpy(peer_public_y, p, sizeof(peer_public_y));
        p += sizeof(peer_public_y);
        sprd_vnd_set_p256_dhkey(address, peer_public_x, peer_public_y);

    } else if (sub_event == HCI_SUPER_SSP_REFRESH_REQUEST_EVT) {
        sprd_vend_ssp_key_init();
    }
}

void sprd_vnd_ssp_init(void)
{
    BDBG("sprd_vnd_ssp_init");
    alogo_init();
    sprd_vnd_set_super_ssp_enable(1);
}
