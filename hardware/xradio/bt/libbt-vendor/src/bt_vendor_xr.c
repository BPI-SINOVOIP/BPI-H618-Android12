/******************************************************************************
 *
  * Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_vendor_xr.c
 *
 *  Description:   Xradio vendor specific library implementation
 *
 ******************************************************************************/

#undef NDEBUG
#define LOG_TAG "bt_vendor"
#include <utils/Log.h>
#include <string.h>
#include <cutils/properties.h>

#include "bt_vendor_xr.h"
#include "upio.h"
#include "userial_vendor.h"

#define CASE_RETURN_STR(const) case const: return #const;

#ifndef BTVND_DBG
#define BTVND_DBG FALSE
#endif

#if (BTVND_DBG == TRUE)
#define BTVNDDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTVNDDBG(param, ...) {}
#endif

#ifndef PROPERTY_DRIVER_VERSION_PATH
#define PROPERTY_DRIVER_VERSION_PATH         "bluetooth.driver.version"
#endif

/******************************************************************************
**  Externs
******************************************************************************/
extern void vnd_load_conf(const char *p_path);
extern void hw_config_start(void);
extern uint32_t hw_lpm_get_idle_timeout(void);
extern uint8_t  hw_lpm_enable(uint8_t turn_on);
extern uint8_t  hw_lpm_set_wake_state(uint8_t wake_assert);
#if (HW_END_WITH_HCI_RESET == TRUE)
extern void hw_epilog_process(void);
#endif
extern void hw_a2dp_coex_start(uint8_t ctrl_cmd);
extern int xr_socket_open(int fd);
extern void xr_socket_close(void);

/******************************************************************************
**  Variables
******************************************************************************/

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************************************************************************
**  Local type definitions
******************************************************************************/

/******************************************************************************
**  Static Variables
******************************************************************************/

static const tUSERIAL_CFG userial_init_cfg = {
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200,
    USERIAL_HW_FLOW_CTRL_OFF
};

/******************************************************************************
**  Functions
******************************************************************************/
const char *dump_vendor_opcode_name(bt_vendor_opcode_t opcode)
{
    int op = (int)opcode;
    switch (op) {
        CASE_RETURN_STR(BT_VND_OP_POWER_CTRL)
        CASE_RETURN_STR(BT_VND_OP_FW_CFG)
        CASE_RETURN_STR(BT_VND_OP_SCO_CFG)
        CASE_RETURN_STR(BT_VND_OP_USERIAL_OPEN)
        CASE_RETURN_STR(BT_VND_OP_USERIAL_CLOSE)
        CASE_RETURN_STR(BT_VND_OP_GET_LPM_IDLE_TIMEOUT)
        CASE_RETURN_STR(BT_VND_OP_LPM_SET_MODE)
        CASE_RETURN_STR(BT_VND_OP_LPM_WAKE_SET_STATE)
        CASE_RETURN_STR(BT_VND_OP_EPILOG)
        CASE_RETURN_STR(BT_VND_OP_A2DP_OFFLOAD_START)
        CASE_RETURN_STR(BT_VND_OP_A2DP_OFFLOAD_STOP)
        default: return "UNKNOWN_OPCODE";
    }
}

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    char ver[128];
    int  idx = 0;

    BTVNDDBG("bt_vendor_xr - init()");

#ifdef BUILD_VERSION
    idx = sprintf(&ver[idx], "%s, %s, ", BUILD_VERSION, BUILD_TIME);
#endif
#ifdef GIT_VERSION
    idx += sprintf(&ver[idx], "%s", GIT_VERSION);
#endif
    ver[idx] = '\0';

    ALOGI("bt driver version:%s", ver);
    if (property_set(PROPERTY_DRIVER_VERSION_PATH, ver) < 0)
        ALOGE("fail to set bt version information in prop %s", PROPERTY_DRIVER_VERSION_PATH);

    if (p_cb == NULL) {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    userial_vendor_init();
    upio_init();

    vnd_load_conf(VENDOR_LIB_CONF_FILE);

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);

    BTVNDDBG("vnd_local_bd_addr:%02X:%02X:%02X:%02X:%02X:%02X", \
        local_bdaddr[0], local_bdaddr[1], local_bdaddr[2],
        local_bdaddr[3], local_bdaddr[4], local_bdaddr[5]);

    property_set("bluetooth.enable_timeout_ms", "8000");

    return 0;
}


/* Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;

    // BTVNDDBG("vendor op:%s", dump_vendor_opcode_name(opcode));
    int op = (int)opcode;

    switch (op) {
        case BT_VND_OP_POWER_CTRL:
            {
                int *state = (int *) param;
                if (*state == BT_VND_PWR_OFF) {
                    upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
                    BTVNDDBG("set power off");
                    upio_set_btwake(0);
                    BTVNDDBG("Set BT WAKE IO Low");
                } else if (*state == BT_VND_PWR_ON) {
                    upio_set_bluetooth_power(UPIO_BT_POWER_ON);
                    BTVNDDBG("set power on");
                    upio_set_btwake(LPM_BT_WAKE_POLARITY);
                    BTVNDDBG("Assert BT WAKE IO");
                }
            }
            break;

        case BT_VND_OP_FW_CFG:
            hw_config_start();
            break;

        case BT_VND_OP_SCO_CFG:
            retval = -1;
            break;

        case BT_VND_OP_USERIAL_OPEN:
            {
                int (*fd_array)[] = (int (*)[]) param;
                int fd, idx;
                fd = userial_vendor_open((tUSERIAL_CFG *) &userial_init_cfg);

                if (fd < 0) {
                    retval = 0;
                    break;
                }

                fd = xr_socket_open(fd);
                if (fd != -1) {
                    for (idx=0; idx < CH_MAX; idx++)
                        (*fd_array)[idx] = fd;

                    retval = 1;
                }
            }
            break;

        case BT_VND_OP_USERIAL_CLOSE:
            userial_vendor_close();
            xr_socket_close();
            break;

        case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
            {
                uint32_t *timeout_ms = (uint32_t *)param;
                *timeout_ms = hw_lpm_get_idle_timeout();
            }
            break;

        case BT_VND_OP_LPM_SET_MODE:
            {
                uint8_t *mode = (uint8_t *)param;
                retval = hw_lpm_enable(*mode);
            }
            break;

        case BT_VND_OP_LPM_WAKE_SET_STATE:
            {
#if (LPM_SLEEP_MODE == TRUE)
                uint8_t *state = (uint8_t *)param;
                uint8_t wakeup_set = (*state == BT_VND_LPM_WAKE_ASSERT) ? TRUE : FALSE;
                retval = hw_lpm_set_wake_state(wakeup_set);
#else
                retval = -1;
#endif
            }
            break;
        case BT_VND_OP_EPILOG:
#if (HW_END_WITH_HCI_RESET == FALSE)
            if (bt_vendor_cbacks)
                bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
#else
            hw_epilog_process();
#endif
            break;

        case BT_VND_OP_SET_AUDIO_STATE:
            retval = 0;
            break;
        case BT_VND_OP_A2DP_OFFLOAD_START:
            break;
        case BT_VND_OP_A2DP_OFFLOAD_STOP:
            break;
        default:
            break;
    }

    return retval;
}

/* Closes the interface */
static void cleanup(void)
{
    BTVNDDBG("bt_vendor_xr - cleanup()");

    upio_cleanup();

    bt_vendor_cbacks = NULL;
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup
};
