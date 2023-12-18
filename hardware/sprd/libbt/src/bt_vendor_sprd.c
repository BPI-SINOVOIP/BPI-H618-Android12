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

/******************************************************************************
 *
 *  Filename:      bt_vendor_brcm.c
 *
 *  Description:   Spreadtrum vendor specific library implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <signal.h>
#include <unistd.h>
#include <log/log.h>
#include <string.h>
#include <sys/stat.h>
#include "bt_vendor_sprd.h"
#include "upio.h"
#include "userial_vendor.h"
#include "comm.h"
#include "sitm.h"

#include "bt_vendor_lib.h"
#include "bt_vendor_sprd_hci.h"
#include "bt_vendor_sprd_a2dp.h"
#include "bt_vendor_sprd_ssp.h"
//#include "btsnoop_sprd.h"
#include <semaphore.h>
#include <time.h>

#ifndef BTVND_DBG
#define BTVND_DBG TRUE
#endif

#if (BTVND_DBG == TRUE)
#define BTVNDDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTVNDDBG(param, ...) {}
#endif

#define CASE_RETURN_STR(const) case const: return #const;
static sem_t bt_sprd_spec_sem;

/******************************************************************************
**  Externs
******************************************************************************/

int hw_preload_pskey(void* arg);
uint8_t hw_lpm_enable(uint8_t turn_on);
uint32_t hw_lpm_get_idle_timeout(void);
void hw_lpm_set_wake_state(uint8_t wake_assert);
void vnd_load_conf(const char* p_path);
#if (HW_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif

/******************************************************************************
**  Variables
******************************************************************************/

bt_vendor_callbacks_t* bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************************************************************************
**  Local type definitions
******************************************************************************/

/******************************************************************************
**  Static Variables
******************************************************************************/

static const tUSERIAL_CFG userial_init_cfg = {
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_3M
};

static const bt_adapter_module_t* adapter_module;

/******************************************************************************
**  Functions
******************************************************************************/
int check_access(const char *name)
{
    int ret, count = 0;
    while (count < 1000) {
        if (!access(name, R_OK | W_OK)) {
            ALOGD("%s, %s R/W access OK after wait %dms", __func__, name, count);
            return 0;
        }
        count++;
        usleep(1000);
    }
    ALOGD("%s, %s R/W access ERR after wait %dms", __func__, name, count);
    return -1;
}

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static void terminate(int sig)
{
    ALOGD("terminate: %d", sig);
    userial_vendor_close();
    usleep(500 * 1000);
    ALOGD("terminate exit");
    upio_set(UPIO_BT_WAKE, UPIO_DEASSERT, 0);
    kill(getpid(), SIGKILL);
}

static int init(const bt_vendor_callbacks_t* p_cb,
                unsigned char* local_bdaddr)
{
    ALOGI("init");

    /* setprop vendor.driver.lpm.load = 1 to retrigger node permission set. */
    property_set("vendor.driver.lpm.load", "1");

    if (p_cb == NULL) {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);

    userial_vendor_init();
    upio_init();
    sprd_vendor_hci_init();

    adapter_module = get_adapter_module();

    if (adapter_module->init != NULL) {
        adapter_module->init();
    }

    ALOGD("%s start up", adapter_module->name);

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t*)p_cb;

    signal(SIGINT, terminate);
    return 0;
}

static const char* dump_opcode(int opcode)
{
    switch (opcode) {
        CASE_RETURN_STR(BT_VND_OP_POWER_CTRL)
        CASE_RETURN_STR(BT_VND_OP_FW_CFG)
        CASE_RETURN_STR(BT_VND_OP_SCO_CFG)
        CASE_RETURN_STR(BT_VND_OP_USERIAL_OPEN)
        CASE_RETURN_STR(BT_VND_OP_USERIAL_CLOSE)
        CASE_RETURN_STR(BT_VND_OP_GET_LPM_IDLE_TIMEOUT)
        CASE_RETURN_STR(BT_VND_OP_LPM_SET_MODE)
        CASE_RETURN_STR(BT_VND_OP_LPM_WAKE_SET_STATE)
        CASE_RETURN_STR(BT_VND_OP_SET_AUDIO_STATE)
        CASE_RETURN_STR(BT_VND_OP_EPILOG)
        CASE_RETURN_STR(BT_VND_OP_A2DP_OFFLOAD_START)
        CASE_RETURN_STR(BT_VND_OP_A2DP_OFFLOAD_STOP)
#if (defined(SPRD_FEATURE_VND_OP_EVENT) && SPRD_FEATURE_VND_OP_EVENT == TRUE)
        CASE_RETURN_STR(BT_VND_OP_EVENT_CALLBACK)
#endif

    default:
        return "unknown status code";
    }
}

void sprd_bt_power_ctrl(unsigned char on)
{
    if (on == BT_VND_PWR_ON) {
        upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
    } else if (on == UPIO_BT_POWER_OFF) {
        upio_set_bluetooth_power(UPIO_BT_POWER_ON);
    }
}

void sprd_bt_lpm_wake_up(void)
{
    upio_set(UPIO_LPM_MODE, UPIO_ASSERT, 0);
}

int sprd_bt_bqb_init(void)
{
    int fd;
    fd = userial_vendor_open((tUSERIAL_CFG*)&userial_init_cfg);
    return fd;
}

void bt_sprd_spec_cb(bt_vendor_op_result_t result) {
    ALOGD("%s result: %d", __func__, result);
    sem_post(&bt_sprd_spec_sem);
}

/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void* param)
{
    int retval = 0;

//    BTVNDDBG("op for %s", dump_opcode(opcode));

    switch (opcode) {
    case BT_VND_OP_POWER_CTRL: {
        int* state = (int*)param;
        if (*state == BT_VND_PWR_OFF) {
            upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
        } else if (*state == BT_VND_PWR_ON) {
            upio_set_bluetooth_power(UPIO_BT_POWER_ON);
        }
    }
    break;

    case BT_VND_OP_FW_CFG: {
#if (BT_VND_STACK_PRELOAD == TRUE)
        hw_preload_pskey(NULL);
#else
        bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
#endif
    }
    break;

    case BT_VND_OP_SCO_CFG: {
        bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);
        retval = 0;
    }
    break;

    case BT_VND_OP_USERIAL_OPEN: {
        int(*fd_array)[] = (int(*)[])param;
        int fd, idx;
        fd = userial_vendor_open((tUSERIAL_CFG*)&userial_init_cfg);
#if (BT_VND_STACK_PRELOAD == FALSE)
        retval = hw_preload_pskey(&fd);
        if (retval < 0) {
            BTVNDDBG("preload pskey failed");
            userial_vendor_close();
            fd = -1;
        }
#endif

#if (BT_SITM_SERVICE == TRUE)
        fd = sitm_server_start_up(fd);
#endif

        if (fd != -1) {
            for (idx = 0; idx < CH_MAX; idx++) (*fd_array)[idx] = fd;

            retval = 1;
        }
        /* retval contains numbers of open fd of HCI channels */
    }
    break;

    case BT_VND_OP_USERIAL_CLOSE: {
        userial_vendor_close();
#if (BT_SITM_SERVICE == TRUE)
        sitm_server_shut_down();
#endif
        upio_set(UPIO_BT_WAKE, UPIO_DEASSERT, 0);
    }
    break;

    case BT_VND_OP_GET_LPM_IDLE_TIMEOUT: {
        uint32_t* timeout_ms = (uint32_t*)param;
        *timeout_ms = hw_lpm_get_idle_timeout();
    }
    break;

    case BT_VND_OP_LPM_SET_MODE: {
        uint8_t* mode = (uint8_t*)param;
        retval = hw_lpm_enable(*mode);
        if (*mode == 0) {
            struct timeval time_now;
            struct timespec act_timeout;
            sem_init(&bt_sprd_spec_sem, 0, 0);
            hw_epilog_process();
            gettimeofday(&time_now, NULL);
            act_timeout.tv_sec = time_now.tv_sec + 5;
            act_timeout.tv_nsec = time_now.tv_usec * 1000;
            if (sem_timedwait(&bt_sprd_spec_sem, &act_timeout) < 0) {
                ALOGE("wait for firmware turn off faild");
            }
            sem_destroy(&bt_sprd_spec_sem);
        }
    }
    break;

    case BT_VND_OP_LPM_WAKE_SET_STATE: {
        uint8_t* state = (uint8_t*)param;
        uint8_t wake_assert = (*state == BT_VND_LPM_WAKE_ASSERT) ? TRUE : FALSE;

        hw_lpm_set_wake_state(wake_assert);
    }
    break;

    case BT_VND_OP_SET_AUDIO_STATE: {
        retval = -1;
    }
    break;

    case BT_VND_OP_EPILOG: {
#if (HW_END_WITH_HCI_RESET == FALSE)
        if (bt_vendor_cbacks) {
            bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
        }
#else
        hw_epilog_process();
#endif
    }
    break;

    case BT_VND_OP_A2DP_OFFLOAD_START:
    case BT_VND_OP_A2DP_OFFLOAD_STOP:
    break;

#if (defined(SPRD_FEATURE_VND_OP_EVENT) && SPRD_FEATURE_VND_OP_EVENT == TRUE)
    case BT_VND_OP_EVENT_CALLBACK: {
        uint8_t *p = (uint8_t*)param;
        BT_HDR *buf = (BT_HDR*)(p + 2);
        buf->len = buf->len - 2;
        sprd_vse_cback(buf->len, buf->data);
    }
    break;
#endif

    }
    return retval;
}

/** Closes the interface */
static void cleanup(void)
{
    BTVNDDBG("cleanup");

    upio_cleanup();

    bt_vendor_cbacks = NULL;

    sprd_vendor_hci_cleanup();

}


// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup,
};
