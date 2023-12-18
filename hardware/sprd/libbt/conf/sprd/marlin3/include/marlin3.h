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
#ifndef LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_
#define LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_

#include "bt_vendor_sprd.h"
#include "bt_vendor_sprd_ssp.h"

#define PSKEY_PRELOAD_SIZE 0x04
#define PSKEY_PREAMBLE_SIZE 0xD8
#define HCI_PSKEY 0xFCA0
#define HCI_RF_PARA 0xFCA2
#define HCI_VSC_ENABLE_COMMMAND 0xFCA1
#define START_STOP_CMD_SIZE 3
#define FW_NODE_BYTE 6
#define FW_DATE_Y_BYTE 10
#define FW_DATE_M_BYTE 9
#define FW_DATE_D_BYTE 8
#define FW_DEFAULT_PROP "FFFF.FFFF.FF.FF"
#define FW_PROP_NAME "rw.vendor.bluetooth.fw.ver"

/*  start bt with dual/classic/ble mode */
enum { DUAL_MODE = 0, CLASSIC_MODE, LE_MODE };
/*  vsc_enable_command parameter : enable or disable */
enum { DISABLE_BT = 0, ENABLE_BT };

typedef struct {
    uint32_t  device_class;
    uint8_t  feature_set[16];
    uint8_t  device_addr[6];
    uint16_t  comp_id;
    uint8_t g_sys_uart0_communication_supported;
    uint8_t cp2_log_mode;
    uint8_t LogLevel;
    uint8_t g_central_or_perpheral;
    uint16_t Log_BitMask;
    uint8_t super_ssp_enable;
    uint8_t common_rfu_b3;
    uint32_t common_rfu_w[2];
    uint32_t le_rfu_w[2];
    uint32_t lmp_rfu_w[2];
    uint32_t lc_rfu_w[2];
    uint16_t g_wbs_nv_117;
    uint16_t g_wbs_nv_118;
    uint16_t g_nbv_nv_117;
    uint16_t g_nbv_nv_118;
    uint8_t g_sys_sco_transmit_mode;
    uint8_t audio_rfu_b1;
    uint8_t audio_rfu_b2;
    uint8_t audio_rfu_b3;
    uint32_t audio_rfu_w[2];
    uint8_t g_sys_sleep_in_standby_supported;
    uint8_t g_sys_sleep_master_supported;
    uint8_t g_sys_sleep_slave_supported;
    uint8_t power_rfu_b1;
    uint32_t power_rfu_w[2];
    uint32_t win_ext;
    uint8_t edr_tx_edr_delay;
    uint8_t edr_rx_edr_delay;
    uint8_t tx_delay;
    uint8_t rx_delay;
    uint32_t bb_rfu_w[2];
    uint8_t agc_mode;
    uint8_t diff_or_eq;
    uint8_t ramp_mode;
    uint8_t modem_rfu_b1;
    uint32_t modem_rfu_w[2];
    uint32_t BQB_BitMask_1;
    uint32_t BQB_BitMask_2;
    uint16_t bt_coex_threshold[8];
    uint32_t other_rfu_w[6];
} pskey_config_t;

typedef struct {
    uint16_t g_GainValue_A[6];
    uint16_t g_ClassicPowerValue_A[10];
    uint16_t g_LEPowerValue_A[16];
    uint16_t g_BRChannelpwrvalue_A[8];
    uint16_t g_EDRChannelpwrvalue_A[8];
    uint16_t g_LEChannelpwrvalue_A[8];
    uint16_t g_GainValue_B[6];
    uint16_t g_ClassicPowerValue_B[10];
    uint16_t g_LEPowerValue_B[16];
    uint16_t g_BRChannelpwrvalue_B[8];
    uint16_t g_EDRChannelpwrvalue_B[8];
    uint16_t g_LEChannelpwrvalue_B[8];
    uint16_t LE_fix_powerword;
    uint8_t Classic_pc_by_channel;
    uint8_t LE_pc_by_channel;
    uint8_t RF_switch_mode;
    uint8_t Data_Capture_Mode;
    uint8_t Analog_IQ_Debug_Mode;
    uint8_t RF_common_rfu_b3;
    uint32_t RF_common_rfu_w[5];
}rf_config_t;


const bt_adapter_module_t *get_adapter_module(void);

#endif  // LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_
