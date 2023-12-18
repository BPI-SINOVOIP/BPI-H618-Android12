/*
 * Copyright (C) 2019-2020 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "phNxpNciHal_extOperations.h"
#include <phNxpLog.h>
#include <phNxpNciHal_ext.h>
#include "phNfcCommon.h"
#include "phNxpNciHal_IoctlOperations.h"

#define NCI_HEADER_SIZE 3
#define NCI_SE_CMD_LEN 4
nxp_nfc_config_ext_t config_ext;
/******************************************************************************
 * Function         phNxpNciHal_updateAutonomousPwrState
 *
 * Description      This function can be used to update autonomous pwr state.
 *                  num: value to check  switch off bit is set or not.
 *
 * Returns          uint8_t
 *
 ******************************************************************************/
uint8_t phNxpNciHal_updateAutonomousPwrState(uint8_t num) {
  if ((config_ext.autonomous_mode == true) &&
      ((num & SWITCH_OFF_MASK) == SWITCH_OFF_MASK)) {
    num = (num | AUTONOMOUS_SCREEN_OFF_LOCK_MASK);
  }
  return num;
}
/******************************************************************************
 * Function         phNxpNciHal_setAutonomousMode
 *
 * Description      This function can be used to set NFCC in autonomous mode
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *                  or NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setAutonomousMode() {
  if (nfcFL.chipType != sn100u) {
    NXPLOG_NCIHAL_D("%s : Not applicable for chipType %d", __func__,
                    nfcFL.chipType);
    return NFCSTATUS_SUCCESS;
  }
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  uint8_t autonomous_mode_value = 0x01;
  if (config_ext.autonomous_mode == true) autonomous_mode_value = 0x02;

  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  mEEPROM_info.buffer = (uint8_t*)&autonomous_mode_value;
  mEEPROM_info.bufflen = sizeof(autonomous_mode_value);
  mEEPROM_info.request_type = EEPROM_AUTONOMOUS_MODE;

  return request_EEPROM(&mEEPROM_info);
}
/******************************************************************************
 * Function         phNxpNciHal_setGuardTimer
 *
 * Description      This function can be used to set nfcc Guard timer
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setGuardTimer() {
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};

  if (config_ext.autonomous_mode != true) config_ext.guard_timer_value = 0x00;

  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  mEEPROM_info.buffer = &config_ext.guard_timer_value;
  mEEPROM_info.bufflen = sizeof(config_ext.guard_timer_value);
  mEEPROM_info.request_type = EEPROM_GUARD_TIMER;

  return request_EEPROM(&mEEPROM_info);
}

/******************************************************************************
 * Function         get_system_property_se_type
 *
 * Description      This will read NFCEE status from system properties
 *                  and returns status.
 *
 * Returns          NFCEE enabled(0x01)/disabled(0x00)
 *
 ******************************************************************************/
static int8_t get_system_property_se_type(uint8_t se_type) {
  int8_t retVal = -1;
  char valueStr[PROPERTY_VALUE_MAX] = {0};
  if (se_type >= NUM_SE_TYPES) return retVal;
  int len = 0;
  switch (se_type) {
    case SE_TYPE_ESE:
      len = property_get("nfc.product.support.ese", valueStr, "");
      break;
    case SE_TYPE_UICC:
      len = property_get("nfc.product.support.uicc", valueStr, "");
      break;
    case SE_TYPE_UICC2:
      len = property_get("nfc.product.support.uicc2", valueStr, "");
      break;
  }
  if (strlen(valueStr) == 0 || len <= 0) {
    return retVal;
  }
  retVal = atoi(valueStr);
  return retVal;
}

/******************************************************************************
 * Function         phNxpNciHal_read_and_update_se_state
 *
 * Description      This will read NFCEE status from system properties
 *                  and update to NFCC to enable/disable.
 *
 * Returns          none
 *
 ******************************************************************************/
void phNxpNciHal_read_and_update_se_state() {
  NFCSTATUS status = NFCSTATUS_FAILED;
  int16_t i = 0;
  int8_t val = -1;
  int16_t num_se = 0;
  uint8_t retry_cnt = 0;
  int8_t values[NUM_SE_TYPES];

  for (i = 0; i < NUM_SE_TYPES; i++) {
    val = get_system_property_se_type(i);
    switch (i) {
      case SE_TYPE_ESE:
        NXPLOG_NCIHAL_D("Get property : SUPPORT_ESE %d", val);
        values[SE_TYPE_ESE] = val;
        if (val > -1) {
          num_se++;
        }
        break;
      case SE_TYPE_UICC:
        NXPLOG_NCIHAL_D("Get property : SUPPORT_UICC %d", val);
        values[SE_TYPE_UICC] = val;
        if (val > -1) {
          num_se++;
        }
        break;
      case SE_TYPE_UICC2:
        values[SE_TYPE_UICC2] = val;
        if (val > -1) {
          num_se++;
        }
        NXPLOG_NCIHAL_D("Get property : SUPPORT_UICC2 %d", val);
        break;
    }
  }
  if (num_se < 1) {
    return;
  }
  uint8_t set_cfg_cmd[NCI_HEADER_SIZE + 1 +
                      (num_se * NCI_SE_CMD_LEN)];  // 1 for Number of Argument
  uint8_t* index = &set_cfg_cmd[0];
  *index++ = NCI_MT_CMD;
  *index++ = NXP_CORE_SET_CONFIG_CMD;
  *index++ = (num_se * NCI_SE_CMD_LEN) + 1;
  *index++ = num_se;
  for (i = 0; i < NUM_SE_TYPES; i++) {
    switch (i) {
      case SE_TYPE_ESE:
        if (values[SE_TYPE_ESE] > -1) {
          *index++ = 0xA0;
          *index++ = 0xED;
          *index++ = 0x01;
          *index++ = values[SE_TYPE_ESE];
        }
        break;
      case SE_TYPE_UICC:
        if (values[SE_TYPE_UICC] > -1) {
          *index++ = 0xA0;
          *index++ = 0xEC;
          *index++ = 0x01;
          *index++ = values[SE_TYPE_UICC];
        }
        break;
      case SE_TYPE_UICC2:
        if (values[SE_TYPE_UICC2] > -1) {
          *index++ = 0xA0;
          *index++ = 0xD4;
          *index++ = 0x01;
          *index++ = values[SE_TYPE_UICC2];
        }
        break;
    }
  }

  while (status != NFCSTATUS_SUCCESS && retry_cnt < 3) {
    status = phNxpNciHal_send_ext_cmd(sizeof(set_cfg_cmd), set_cfg_cmd);
    retry_cnt++;
    NXPLOG_NCIHAL_E("Get Cfg Retry cnt=%x", retry_cnt);
  }
}

/******************************************************************************
 * Function         phNxpNciHal_read_fw_dw_status
 *
 * Description      This will read the value of fw download status flag
 *                  from eeprom
 *
 * Parameters       value - this parameter will be updated with the flag
 *                  value from eeprom.
 *
 * Returns          status of the read
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_read_fw_dw_status(uint8_t& value) {
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  mEEPROM_info.buffer = &value;
  mEEPROM_info.bufflen = sizeof(value);
  mEEPROM_info.request_type = EEPROM_FW_DWNLD;
  mEEPROM_info.request_mode = GET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}

/******************************************************************************
 * Function         phNxpNciHal_write_fw_dw_status
 *
 * Description      This will update value of fw download status flag
 *                  to eeprom
 *
 * Parameters       value - this value will be updated to eeprom flag.
 *
 * Returns          status of the write
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_write_fw_dw_status(uint8_t value) {
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  mEEPROM_info.buffer = &value;
  mEEPROM_info.bufflen = sizeof(value);
  mEEPROM_info.request_type = EEPROM_FW_DWNLD;
  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}

/******************************************************************************
 * Function         phNxpNciHal_get_uicc_hci_params
 *
 * Description      This will read the UICC HCI param values
 *                  from eeprom
 *
 * Parameters       value - this parameter will be updated with the flag
 *                  value from eeprom.
 *
 * Returns          status of the read
 *
 ******************************************************************************/
NFCSTATUS
phNxpNciHal_get_uicc_hci_params(std::vector<uint8_t>& ptr, uint8_t bufflen,
                                phNxpNci_EEPROM_request_type_t uiccType) {
  if (nfcFL.chipType < sn220u) {
    NXPLOG_NCIHAL_E("%s Not supported", __func__);
    return NFCSTATUS_SUCCESS;
  }
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  mEEPROM_info.buffer = &ptr[0];
  mEEPROM_info.bufflen = bufflen;
  mEEPROM_info.request_type = uiccType;
  mEEPROM_info.request_mode = GET_EEPROM_DATA;
  NFCSTATUS status = request_EEPROM(&mEEPROM_info);
  ptr.resize(mEEPROM_info.bufflen);
  return status;
}

/******************************************************************************
 * Function         phNxpNciHal_set_uicc_hci_params
 *
 * Description      This will update the UICC HCI param values
 *                  to eeprom
 *
 * Parameters       value - this value will be updated to eeprom flag.
 *
 * Returns          status of the write
 *
 *****************************************************************************/
NFCSTATUS
phNxpNciHal_set_uicc_hci_params(std::vector<uint8_t>& ptr, uint8_t bufflen,
                                phNxpNci_EEPROM_request_type_t uiccType) {
  if (nfcFL.chipType < sn220u) {
    NXPLOG_NCIHAL_E("%s Not supported", __func__);
    return NFCSTATUS_SUCCESS;
  }
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  mEEPROM_info.buffer = &ptr[0];
  mEEPROM_info.bufflen = bufflen;
  mEEPROM_info.request_type = uiccType;
  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}

/*****************************************************************************
 * Function         phNxpNciHal_send_get_cfg
 *
 * Description      This function is called to get the configurations from
 * EEPROM
 *
 * Params           cmd_get_cfg, Buffer to get the get command
 *                  cmd_len,     Length of the command
 * Returns          SUCCESS/FAILURE
 *
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_send_get_cfg(const uint8_t* cmd_get_cfg, long cmd_len) {
  NXPLOG_NCIHAL_D("%s Enter", __func__);
  NFCSTATUS status = NFCSTATUS_FAILED;
  uint8_t retry_cnt = 0;

  if (cmd_get_cfg == NULL || cmd_len <= NCI_GET_CONFI_MIN_LEN) {
    NXPLOG_NCIHAL_E("%s invalid command..! returning... ", __func__);
    return status;
  }

  do {
    status = phNxpNciHal_send_ext_cmd(cmd_len, (uint8_t*)cmd_get_cfg);
  } while ((status != NFCSTATUS_SUCCESS) &&
           (retry_cnt++ < NXP_MAX_RETRY_COUNT));

  NXPLOG_NCIHAL_D("%s status : 0x%02X", __func__, status);
  return status;
}

/*****************************************************************************
 * Function         phNxpNciHal_configure_merge_sak
 *
 * Description      This function is called to apply iso_dep sak merge settings
 *                  as per the config option NAME_NXP_ISO_DEP_MERGE_SAK
 *
 * Params           None

 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_configure_merge_sak() {
  if (nfcFL.chipType != sn100u) {
    NXPLOG_NCIHAL_D("%s : Not applicable for chipType %d", __func__,
                    nfcFL.chipType);
    return NFCSTATUS_SUCCESS;
  }
  long retlen = 0;
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};
  NXPLOG_NCIHAL_D("Performing ISODEP sak merge settings");
  uint8_t val = 0;

  if (!GetNxpNumValue(NAME_NXP_ISO_DEP_MERGE_SAK, (void*)&retlen,
                      sizeof(retlen))) {
    retlen = 0x01;
    NXPLOG_NCIHAL_D(
        "ISO_DEP_MERGE_SAK not found. default shall be enabled : 0x%02lx",
        retlen);
  }
  val = (uint8_t)retlen;
  mEEPROM_info.buffer = &val;
  mEEPROM_info.bufflen = sizeof(val);
  mEEPROM_info.request_type = EEPROM_ISODEP_MERGE_SAK;
  mEEPROM_info.request_mode = SET_EEPROM_DATA;
  return request_EEPROM(&mEEPROM_info);
}
#if (NXP_EXTNS == TRUE && NXP_SRD == TRUE)
/******************************************************************************
 * Function         phNxpNciHal_setSrdtimeout
 *
 * Description      This function can be used to set srd SRD Timeout.
 *
 * Returns          NFCSTATUS_FAILED or NFCSTATUS_SUCCESS or
 *                  NFCSTATUS_FEATURE_NOT_SUPPORTED
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_setSrdtimeout() {
  long retlen = 0;
  uint8_t* buffer = nullptr;
  long bufflen = 260;
  static const int NXP_SRD_TIMEOUT_BUF_LEN = 2;
  static const uint16_t TIMEOUT_MASK = 0xFFFF;
  static const uint16_t MAX_TIMEOUT_VALUE = 0x0258;
  uint16_t isValid_timeout;
  uint8_t timeout_buffer[NXP_SRD_TIMEOUT_BUF_LEN];
  NFCSTATUS status = NFCSTATUS_FEATURE_NOT_SUPPORTED;
  phNxpNci_EEPROM_info_t mEEPROM_info = {.request_mode = 0};

  NXPLOG_NCIHAL_D("Performing SRD Timeout settings");

  buffer = (uint8_t*)malloc(bufflen * sizeof(uint8_t));
  if (NULL == buffer) {
    return NFCSTATUS_FAILED;
  }
  memset(buffer, 0x00, bufflen);
  if (GetNxpByteArrayValue(NAME_NXP_SRD_TIMEOUT, (char*)buffer, bufflen,
                           &retlen)) {
    if (retlen == NXP_SRD_TIMEOUT_BUF_LEN) {
      isValid_timeout = ((buffer[1] << 8) & TIMEOUT_MASK);
      isValid_timeout = (isValid_timeout | buffer[0]);
      if (isValid_timeout > MAX_TIMEOUT_VALUE) {
        /*if timeout is setting more than 600 sec
         * than setting to MAX limit 0x0258*/
        buffer[0] = 0x58;
        buffer[1] = 0x02;
      }
      memcpy(&timeout_buffer, buffer, NXP_SRD_TIMEOUT_BUF_LEN);
      mEEPROM_info.buffer = timeout_buffer;
      mEEPROM_info.bufflen = sizeof(timeout_buffer);
      mEEPROM_info.request_type = EEPROM_SRD_TIMEOUT;
      mEEPROM_info.request_mode = SET_EEPROM_DATA;
      status = request_EEPROM(&mEEPROM_info);
    }
  }
  if (buffer != NULL) {
    free(buffer);
    buffer = NULL;
  }

  return status;
}
#endif
