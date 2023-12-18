/******************************************************************************
 *
 *  Copyright (C) 2018 ST Microelectronics S.A.
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
 *
 ******************************************************************************/
#define LOG_TAG "NfcHalFd"
#include "hal_fd.h"
#include <cutils/properties.h>
#include <dlfcn.h>
#include <errno.h>
#include <hardware/nfc.h>
#include <string.h>
#include "android_logmsg.h"
#include "halcore.h"
/* Initialize fw info structure pointer used to access fw info structure */
FWInfo *mFWInfo = NULL;
FILE *mFwFileBin;
FILE *mCustomFileBin;
fpos_t mPos;
fpos_t mPosInit;
uint8_t mBinData[260];
bool mRetry = true;
bool mCustomParamFailed = false;
bool mCustomParamDone = false;
bool mUwbConfigDone = false;
bool mUwbConfigNeeded = false;
bool mGetCustomerField = false;
uint8_t *pCmd;
int mFWRecovCount = 0;
const char *FwType = "generic";
char mApduAuthent[24];

static const uint8_t propNfcModeSetCmdOn[] = {0x2f, 0x02, 0x02, 0x02, 0x01};
static const uint8_t coreInitCmd[] = {0x20, 0x01, 0x02, 0x00, 0x00};
static const uint8_t NciPropNfcFwUpdate[] = {0x2F, 0x02, 0x05, 0x06,
                                             0x00, 0x01, 0x02, 0x03};

static uint8_t ApduEraseNfcKeepAppliAndNdef[] = {
    0x2F, 0x04, 0x16, 0x80, 0x0C, 0x00, 0x00, 0x11, 0x05,
    0x00, 0x23, 0xDF, 0x00, 0x00, 0x23, 0xDF, 0xFF, 0x00,
    0x23, 0xE0, 0x00, 0x00, 0x23, 0xFF, 0xFF};

static const uint8_t ApduExitLoadMode[] = {0x2F, 0x04, 0x06, 0x80, 0xA0,
                                           0x00, 0x00, 0x01, 0x01};
static const uint8_t nciHeaderPropSetUwbConfig[9] = {
    0x2F, 0x02, 0x00, 0x04, 0x00, 0x16, 0x01, 0x00, 0x00};
static const uint8_t nciGetPropConfig[8] = {0x2F, 0x02, 0x05, 0x03,
                                            0x00, 0x06, 0x01, 0x00};
static const uint8_t nciSetPropConfig[9] = {0x2F, 0x02, 0x00, 0x04, 0x00,
                                            0x06, 0x01, 0x00, 0x00};
static uint8_t nciPropSetUwbConfig[128];
static uint8_t nciPropSetConfig_CustomField[64];
hal_fd_state_e mHalFDState = HAL_FD_STATE_AUTHENTICATE;
void SendExitLoadMode(HALHANDLE mmHalHandle);
extern void hal_wrapper_update_complete();

typedef size_t (*STLoadUwbParams)(void *out_buff,
                                  size_t buf_size);

/**
 * Send a HW reset and decode NCI_CORE_RESET_NTF information
 * @param pHwVersion is used to return HW version, part of NCI_CORE_RESET_NTF
 * @param pFwVersion is used to return FW version, part of NCI_CORE_RESET_NTF
 * @param pLoaderVersion is used to return Loader version, part of
 * NCI_CORE_RESET_NTF
 * @param pCustVersion si used to return Customer field value, part of
 * NCI_CORE_RESET_NTF when in router mode
 *
 * @return mode: FT_CLF_MODE_ROUTER if router mode
 *               FT_CLF_MODE_LOADER if loader mode
 *               FT_CLF_MODE_ERROR if Error
 */

int hal_fd_init() {
  uint8_t result = 0;
  char FwPath[256];
  char ConfPath[256];
  char fwBinName[256];
  char fwConfName[256];

  STLOG_HAL_D("  %s - enter", __func__);

  if (!GetStrValue(NAME_STNFC_FW_PATH_STORAGE, (char *)FwPath,
                   sizeof(FwPath))) {
    STLOG_HAL_D(
        "%s - FW path not found in conf. use default location /vendor/firmware "
        "\n", __func__);
    strcpy(FwPath, "/vendor/firmware");
  }

  if (!GetStrValue(NAME_STNFC_FW_BIN_NAME, (char *)fwBinName,
                   sizeof(fwBinName))) {
    STLOG_HAL_D(
        "%s - FW binary file name not found in conf. use default name "
        "/st21nfc_fw.bin \n", __func__);
    strcpy(fwBinName, "/st21nfc_fw.bin");
  }

  if (!GetStrValue(NAME_STNFC_FW_CONF_NAME, (char *)fwConfName,
                   sizeof(fwConfName))) {
    STLOG_HAL_D(
        "%s - FW config file name not found in conf. use default name "
        "/st21nfc_conf.bin \n", __func__);
    strcpy(fwConfName, "/st21nfc_conf.bin");
  }

  // Getting information about FW patch, if any
  strcpy(ConfPath, FwPath);
  strncat(FwPath, fwBinName, sizeof(FwPath) - strlen(FwPath) - 1);
  strncat(ConfPath, fwConfName, sizeof(ConfPath) - strlen(ConfPath) - 1);
  STLOG_HAL_D("%s - FW update binary file = %s", __func__, FwPath);
  STLOG_HAL_D("%s - FW config binary file = %s", __func__, ConfPath);

  // Initializing structure holding FW patch details
  mFWInfo = (FWInfo *)malloc(sizeof(FWInfo));

  if (mFWInfo == NULL) {
    result = 0;
  }

  memset(mFWInfo, 0, sizeof(FWInfo));

  mFwFileBin = NULL;
  mCustomFileBin = NULL;

  // Check if FW patch binary file is present
  // If not, get recovery FW patch file
  if ((mFwFileBin = fopen((char *)FwPath, "r")) == NULL) {
    STLOG_HAL_D("%s - %s not detected", __func__, fwBinName);
  } else {
    STLOG_HAL_D("%s - %s file detected\n", __func__, fwBinName);

    result |= FW_PATCH_AVAILABLE;
    fread(mBinData, sizeof(uint8_t), 4, mFwFileBin);
    mFWInfo->patchVersion =
        mBinData[0] << 24 | mBinData[1] << 16 | mBinData[2] << 8 | mBinData[3];

    fread(mApduAuthent, sizeof(uint8_t), 24, mFwFileBin);

    fgetpos(mFwFileBin, &mPosInit);

    STLOG_HAL_D(
        "%s --> st21nfc_fw integrates patch NFC FW version 0x%08X (%s)\n",
        __func__, mFWInfo->patchVersion, FwType);
  }

  if ((mCustomFileBin = fopen((char *)ConfPath, "r")) == NULL) {
    STLOG_HAL_D("%s - st21nfc custom configuration not detected\n", __func__);
  } else {
    STLOG_HAL_D("%s - %s file detected\n", __func__, ConfPath);
    fread(mBinData, sizeof(uint8_t), 2, mCustomFileBin);
    mFWInfo->confVersion = mBinData[0] << 8 | mBinData[1];
    STLOG_HAL_D("%s --> st21nfc_custom configuration version 0x%04X \n",
                __func__, mFWInfo->confVersion);
    result |= FW_CUSTOM_PARAM_AVAILABLE;
  }

  if (ft_CheckUWBConf()) {
    result |= FW_UWB_PARAM_AVAILABLE;
  }

  return result;
}

void hal_fd_close() {
  STLOG_HAL_D("  %s -enter", __func__);
  mCustomParamFailed = false;
  if (mFWInfo != NULL) {
    free(mFWInfo);
    mFWInfo = NULL;
  }
  if (mFwFileBin != NULL) {
    fclose(mFwFileBin);
    mFwFileBin = NULL;
  }
  if (mCustomFileBin != NULL) {
    fclose(mCustomFileBin);
    mCustomFileBin = NULL;
  }
}

/**
 * Send a HW reset and decode NCI_CORE_RESET_NTF information
 * @param pHwVersion is used to return HW version, part of NCI_CORE_RESET_NTF
 * @param pFwVersion is used to return FW version, part of NCI_CORE_RESET_NTF
 * @param pLoaderVersion is used to return Loader version, part of
 * NCI_CORE_RESET_NTF
 * @param pCustVersion si used to return Customer field value, part of
 * NCI_CORE_RESET_NTF when in router mode
 *
 * @return mode: FT_CLF_MODE_ROUTER if router mode
 *               FT_CLF_MODE_LOADER if loader mode
 *               FT_CLF_MODE_ERROR if Error
 */

uint8_t ft_cmd_HwReset(uint8_t *pdata, uint8_t *clf_mode) {
  uint8_t result = 0;

  STLOG_HAL_D("  %s - execution", __func__);

  if ((pdata[1] == 0x0) && (pdata[3] == 0x1)) {
    STLOG_HAL_D("-> Router Mode NCI_CORE_RESET_NTF received after HW Reset");

    /* retrieve HW Version from NCI_CORE_RESET_NTF */
    mFWInfo->hwVersion = pdata[8];
    STLOG_HAL_D("   HwVersion = 0x%02X", mFWInfo->hwVersion);

    /* retrieve FW Version from NCI_CORE_RESET_NTF */
    mFWInfo->fwVersion =
        (pdata[10] << 24) | (pdata[11] << 16) | (pdata[12] << 8) | pdata[13];
    STLOG_HAL_D("   FwVersion = 0x%08X", mFWInfo->fwVersion);

    /* retrieve Loader Version from NCI_CORE_RESET_NTF */
    mFWInfo->loaderVersion = (pdata[14] << 16) | (pdata[15] << 8) | pdata[16];
    STLOG_HAL_D("   LoaderVersion = 0x%06X", mFWInfo->loaderVersion);

    /* retrieve Customer Version from NCI_CORE_RESET_NTF */
    mFWInfo->custVersion = (pdata[31] << 8) | pdata[32];
    STLOG_HAL_D("   CustomerVersion = 0x%04X", mFWInfo->custVersion);

    /* retrieve Uwb param Version from NCI_CORE_RESET_NTF */
    mFWInfo->uwbFwVersion = (pdata[29] << 8) | pdata[30];
    STLOG_HAL_D("   uwbVersion = 0x%04X", mFWInfo->uwbFwVersion);

    *clf_mode = FT_CLF_MODE_ROUTER;
  } else if ((pdata[2] == 0x39) && (pdata[3] == 0xA1)) {
    STLOG_HAL_D("-> Loader Mode NCI_CORE_RESET_NTF received after HW Reset");

    /* deduce HW Version from Factory Loader version */
    if (pdata[16] == 0x01) {
      mFWInfo->hwVersion = 0x05;  // ST54J
    } else if (pdata[16] == 0x02) {
      mFWInfo->hwVersion = 0x04;  // ST21NFCD
    } else {
      mFWInfo->hwVersion = 0x03;  // ST21NFCD
    }
    STLOG_HAL_D("   HwVersion = 0x%02X", mFWInfo->hwVersion);

    /* Identify the Active loader. Normally only one should be detected*/
    if (pdata[11] == 0xA0) {
      mFWInfo->loaderVersion = (pdata[8] << 16) | (pdata[9] << 8) | pdata[10];
      STLOG_HAL_D("         - Most recent loader activated, revision 0x%06X",
                  mFWInfo->loaderVersion);
    }
    if (pdata[15] == 0xA0) {
      mFWInfo->loaderVersion = (pdata[12] << 16) | (pdata[13] << 8) | pdata[14];
      STLOG_HAL_D("         - Least recent loader activated, revision 0x%06X",
                  mFWInfo->loaderVersion);
    }
    if (pdata[19] == 0xA0) {
      mFWInfo->loaderVersion = (pdata[16] << 16) | (pdata[17] << 8) | pdata[18];
      STLOG_HAL_D("         - Factory loader activated, revision 0x%06X",
                  mFWInfo->loaderVersion);
    }

    *clf_mode = FT_CLF_MODE_LOADER;
  } else {
    STLOG_HAL_E(
        "%s --> ERROR: wrong NCI_CORE_RESET_NTF received after HW Reset",
        __func__);
    *clf_mode = FT_CLF_MODE_ERROR;
  }

  // Allow update only for ST54J
  if (mFWInfo->hwVersion == 0x05) {
    if ((mFwFileBin != NULL) && (mFWInfo->patchVersion != mFWInfo->fwVersion)) {
      STLOG_HAL_D("---> Firmware update needed\n");
      result |= FW_UPDATE_NEEDED;
    } else {
      STLOG_HAL_D("---> No Firmware update needed\n");
    }

    if ((mFWInfo->confVersion != 0) &&
        (mFWInfo->custVersion != mFWInfo->confVersion)) {
      STLOG_HAL_D(
          "%s - Need to apply new st21nfc custom configuration settings\n",
          __func__);
      if (!mCustomParamFailed) result |= CONF_UPDATE_NEEDED;
    } else {
      STLOG_HAL_D("%s - No need to apply custom configuration settings\n",
                  __func__);
    }
  }
  if ((mFWInfo->uwbVersion != 0) &&
      (mFWInfo->uwbVersion != mFWInfo->uwbFwVersion)) {
    result |= UWB_CONF_UPDATE_NEEDED;
    STLOG_HAL_D("%s - Need to apply new uwb param configuration \n", __func__);
    mUwbConfigNeeded = true;
  }

  return result;
} /* ft_cmd_HwReset */

void ExitHibernateHandler(HALHANDLE mHalHandle, uint16_t data_len,
                          uint8_t *p_data) {
  STLOG_HAL_D("%s - Enter", __func__);
  if (data_len < 3) {
    STLOG_HAL_E("%s - Error, too short data (%d)", __func__, data_len);
    return;
  }
  switch (p_data[0]) {
    case 0x40:  //
      STLOG_HAL_D("%s - hibernate_exited = %d ", __func__,
                  mFWInfo->hibernate_exited);

      // CORE_INIT_RSP
      if ((p_data[1] == 0x1) && (p_data[3] == 0x0) &&
          (mFWInfo->hibernate_exited == 0)) {
        // Send PROP_NFC_MODE_SET_CMD(ON)
        if (!HalSendDownstream(mHalHandle, propNfcModeSetCmdOn,
                               sizeof(propNfcModeSetCmdOn))) {
          STLOG_HAL_E("%s - SendDownstream failed", __func__);
        }
      } else if ((p_data[1] == 0x1) && (p_data[3] == 0x0) &&
                 (mFWInfo->hibernate_exited == 1)) {
        STLOG_HAL_D(
            "%s - send NCI_PROP_NFC_FW_UPDATE_CMD and use 100 ms timer for "
            "each cmd from here",
            __func__);

        if (!HalSendDownstreamTimer(mHalHandle, NciPropNfcFwUpdate,
                                    sizeof(NciPropNfcFwUpdate),
                                    FW_TIMER_DURATION)) {
          STLOG_HAL_E("%s  SendDownstream failed", __func__);
        }
      } else if (p_data[3] != 0x00) {
        STLOG_HAL_D("%s - Wrong response. Retry HW reset", __func__);
        I2cResetPulse();
        hal_wrapper_set_state(HAL_WRAPPER_STATE_OPEN);
      }
      break;

    case 0x4f:  //
      if ((p_data[1] == 0x02) && (p_data[3] == 0x00) &&
          (mFWInfo->hibernate_exited == 1)) {
        STLOG_HAL_D("%s - NCI_PROP_NFC_FW_RSP : loader mode", __func__);
        I2cResetPulse();
        hal_wrapper_set_state(HAL_WRAPPER_STATE_OPEN);
      } else if (p_data[3] != 0x00) {
        STLOG_HAL_D("%s - Wrong response. Retry HW reset", __func__);
        I2cResetPulse();
        hal_wrapper_set_state(HAL_WRAPPER_STATE_OPEN);
      }
      break;
    case 0x60:  //
      if (p_data[3] == 0x2) {
        STLOG_HAL_D("%s - CORE_RESET_NTF : after core_reset_cmd", __func__);

        if (!HalSendDownstream(mHalHandle, coreInitCmd, sizeof(coreInitCmd))) {
          STLOG_HAL_E("%s - SendDownstream failed", __func__);
        }
      } else if (p_data[3] == 0xa0) {
        mFWInfo->hibernate_exited = 1;
        STLOG_HAL_D("%s - hibernate_exited = %d ", __func__,
                    mFWInfo->hibernate_exited);

        if (!HalSendDownstream(mHalHandle, coreInitCmd, sizeof(coreInitCmd))) {
          STLOG_HAL_E("%s - SendDownstream failed", __func__);
        }
      }
      break;
  }
}

bool ft_CheckUWBConf() {

  char uwbLibName[256];
  STLOG_HAL_D("%s", __func__);

  if (!GetStrValue(NAME_STNFC_UWB_LIB_NAME, (char *)uwbLibName,
                   sizeof(uwbLibName))) {
    STLOG_HAL_D(
        "%s - UWB conf library name not found in conf. use default name ", __func__);
    strcpy(uwbLibName, "/vendor/lib64/libqorvo_uwb_params_nfcc.so");
  }

  STLOG_HAL_D("%s - UWB conf library = %s", __func__, uwbLibName);

  void *stdll = dlopen(uwbLibName, RTLD_NOW);
  if (stdll) {
    STLoadUwbParams fn =
        (STLoadUwbParams)dlsym(stdll, "load_uwb_params_from_files");
  if (fn) {
    size_t lengthOutput =
        fn(nciPropSetUwbConfig + 9, 100);
    STLOG_HAL_D("%s: lengthOutput = %zu", __func__, lengthOutput);
    if (lengthOutput > 0) {
      memcpy(nciPropSetUwbConfig, nciHeaderPropSetUwbConfig, 9);
      nciPropSetUwbConfig[2] = lengthOutput + 6;
      nciPropSetUwbConfig[8] = lengthOutput;
      mFWInfo->uwbVersion =
          nciPropSetUwbConfig[9] << 8 | nciPropSetUwbConfig[10];
      STLOG_HAL_D("%s --> uwb configuration version 0x%04X \n", __func__,
                  mFWInfo->uwbVersion);
      return true;
    } else {
      STLOG_HAL_D("%s: lengthOutput null", __func__);
    }
   }
  } else {
    STLOG_HAL_D("libqorvo_uwb_params_nfcc not found, do nothing.");
  }
  return false;
}
void resetHandlerState() {
  STLOG_HAL_D("%s", __func__);
  mHalFDState = HAL_FD_STATE_AUTHENTICATE;
}

void UpdateHandler(HALHANDLE mHalHandle, uint16_t data_len, uint8_t *p_data) {
  HalSendDownstreamStopTimer(mHalHandle);

  switch (mHalFDState) {
    case HAL_FD_STATE_AUTHENTICATE:
      STLOG_HAL_D("%s - mHalFDState = HAL_FD_STATE_AUTHENTICATE", __func__);

      if ((p_data[data_len - 2] == 0x90) && (p_data[data_len - 1] == 0x00)) {
        STLOG_HAL_D("%s - send APDU_AUTHENTICATION_CMD", __func__);
        if (!HalSendDownstreamTimer(mHalHandle, (uint8_t *)mApduAuthent,
                                    sizeof(mApduAuthent), FW_TIMER_DURATION)) {
          STLOG_HAL_E("%s - SendDownstream failed", __func__);
        }
        mHalFDState = HAL_FD_STATE_ERASE_FLASH;
      } else {
        STLOG_HAL_D("%s - FW flash not succeeded", __func__);
        SendExitLoadMode(mHalHandle);
      }
      break;

    case HAL_FD_STATE_ERASE_FLASH:  // 1
      STLOG_HAL_D("%s - mHalFDState = HAL_FD_STATE_ERASE_FLASH", __func__);

      if ((p_data[0] == 0x4f) && (p_data[1] == 0x04)) {
        if ((p_data[data_len - 2] == 0x90) && (p_data[data_len - 1] == 0x00)) {
          STLOG_HAL_D(
              " %s - send APDU_ERASE_FLASH_CMD (keep appli and NDEF areas)",
              __func__);

          if (!HalSendDownstreamTimer(mHalHandle, ApduEraseNfcKeepAppliAndNdef,
                                      sizeof(ApduEraseNfcKeepAppliAndNdef),
                                      FW_TIMER_DURATION)) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }

          fsetpos(mFwFileBin, &mPosInit);  // reset pos in stream

          mHalFDState = HAL_FD_STATE_SEND_RAW_APDU;

        } else {
          STLOG_HAL_D("%s - FW flash not succeeded", __func__);
          SendExitLoadMode(mHalHandle);
        }
      }
      break;

    case HAL_FD_STATE_SEND_RAW_APDU:  // 3
      STLOG_HAL_D("%s - mHalFDState = HAL_FD_STATE_SEND_RAW_APDU", __func__);
      if ((p_data[0] == 0x4f) && (p_data[1] == 0x04)) {
        if ((p_data[data_len - 2] == 0x90) && (p_data[data_len - 1] == 0x00)) {
          mRetry = true;

          fgetpos(mFwFileBin, &mPos);  // save current position in stream
          if ((fread(mBinData, sizeof(uint8_t), 3, mFwFileBin) == 3) &&
              (fread(mBinData + 3, sizeof(uint8_t), mBinData[2], mFwFileBin) ==
               mBinData[2])) {
            if (!HalSendDownstreamTimer(mHalHandle, mBinData, mBinData[2] + 3,
                                        FW_TIMER_DURATION)) {
              STLOG_HAL_E("%s - SendDownstream failed", __func__);
            }
          } else {
            STLOG_HAL_D("%s - EOF of FW binary", __func__);
            SendExitLoadMode(mHalHandle);
          }
        } else if (mRetry == true) {
          STLOG_HAL_D("%s - Last Tx was NOK. Retry", __func__);
          mRetry = false;
          fsetpos(mFwFileBin, &mPos);
          if ((fread(mBinData, sizeof(uint8_t), 3, mFwFileBin) == 3) &&
              (fread(mBinData + 3, sizeof(uint8_t), mBinData[2], mFwFileBin) ==
               mBinData[2])) {
            if (!HalSendDownstreamTimer(mHalHandle, mBinData, mBinData[2] + 3,
                                        FW_TIMER_DURATION)) {
              STLOG_HAL_E("%s - SendDownstream failed", __func__);
            }
            fgetpos(mFwFileBin, &mPos);  // save current position in stream
          } else {
            STLOG_HAL_D("%s - EOF of FW binary", __func__);
            SendExitLoadMode(mHalHandle);
          }
        } else {
          STLOG_HAL_D("%s - FW flash not succeeded.", __func__);
          I2cResetPulse();
          SendExitLoadMode(mHalHandle);
        }
      }
      break;

    case HAL_FD_STATE_EXIT_APDU:  // 2
      STLOG_HAL_D("%s - mHalFDState = HAL_FD_STATE_EXIT_APDU", __func__);
      if ((p_data[data_len - 2] != 0x90) || (p_data[data_len - 1] != 0x00)) {
        STLOG_HAL_D(
            "%s - Error exiting loader mode, i.e. a problem occured during FW "
            "update", __func__);
      }

      I2cResetPulse();
      hal_wrapper_set_state(HAL_WRAPPER_STATE_OPEN);
      mHalFDState = HAL_FD_STATE_AUTHENTICATE;
      break;

    default:
      STLOG_HAL_D("%s - mHalFDState = unknown", __func__);
      STLOG_HAL_D("%s - FW flash not succeeded", __func__);
      SendExitLoadMode(mHalHandle);
      break;
  }
}

void ApplyCustomParamHandler(HALHANDLE mHalHandle, uint16_t data_len,
                             uint8_t *p_data) {
  STLOG_HAL_D("%s - Enter ", __func__);
  if (data_len < 3) {
    STLOG_HAL_E("%s : Error, too short data (%d)", __func__, data_len);
    return;
  }

  switch (p_data[0]) {
    case 0x40:  //
      // CORE_RESET_RSP
      if ((p_data[1] == 0x0) && (p_data[3] == 0x0)) {
        // do nothing
      } else if ((p_data[1] == 0x1) && (p_data[3] == 0x0)) {
        if (mFWInfo->hibernate_exited == 0) {
          // Send a NFC mode on .
          if (!HalSendDownstream(mHalHandle, propNfcModeSetCmdOn,
                                 sizeof(propNfcModeSetCmdOn))) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }
          // CORE_INIT_RSP
        } else if (mFWInfo->hibernate_exited == 1) {
          if ((fread(mBinData, sizeof(uint8_t), 3, mCustomFileBin)) &&
              (fread(mBinData + 3, sizeof(uint8_t), mBinData[2],
                     mCustomFileBin))) {
            if (!HalSendDownstream(mHalHandle, mBinData, mBinData[2] + 3)) {
              STLOG_HAL_E("%s - SendDownstream failed", __func__);
            }
          }
        }

      } else {
        STLOG_HAL_D("%s - Error in custom param application", __func__);
        mCustomParamFailed = true;
        I2cResetPulse();
        hal_wrapper_set_state(HAL_WRAPPER_STATE_OPEN);
      }
      break;

    case 0x4f:
      if (mFWInfo->hibernate_exited == 1) {
        if ((fread(mBinData, sizeof(uint8_t), 3, mCustomFileBin) == 3) &&
            (fread(mBinData + 3, sizeof(uint8_t), mBinData[2],
                   mCustomFileBin) == mBinData[2])) {
          if (!HalSendDownstream(mHalHandle, mBinData, mBinData[2] + 3)) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }
        } else {
          STLOG_HAL_D("%s - mCustomParamDone = %d", __func__, mCustomParamDone);
          if (!mGetCustomerField) {
            mGetCustomerField = true;
            if (!HalSendDownstream(mHalHandle, nciGetPropConfig,
                                   sizeof(nciGetPropConfig))) {
              STLOG_HAL_E("%s - SendDownstream failed", __func__);
            }
            mGetCustomerField = true;

          } else if (!mCustomParamDone) {

            STLOG_HAL_D("%s - EOF of custom file.", __func__);
            memset(nciPropSetConfig_CustomField, 0x0,
                   sizeof(nciPropSetConfig_CustomField));
            memcpy(nciPropSetConfig_CustomField, nciSetPropConfig, 9);
            nciPropSetConfig_CustomField[8] = p_data[6];
            nciPropSetConfig_CustomField[2] = p_data[6] + 6;
            memcpy(nciPropSetConfig_CustomField + 9, p_data + 7, p_data[6]);
            nciPropSetConfig_CustomField[13] = mFWInfo->uwbFwVersion >> 8;
            nciPropSetConfig_CustomField[14] = mFWInfo->uwbFwVersion;

            if (!HalSendDownstream(mHalHandle, nciPropSetConfig_CustomField,
                                   nciPropSetConfig_CustomField[2] + 3)) {
              STLOG_HAL_E("%s - SendDownstream failed", __func__);
            }

            mCustomParamDone = true;

          } else {
            I2cResetPulse();
            if (mUwbConfigNeeded) {
              mCustomParamDone = false;
              mGetCustomerField = false;
              hal_wrapper_set_state(HAL_WRAPPER_STATE_APPLY_UWB_PARAM);
            }
          }
        }
      }

      // Check if an error has occurred for PROP_SET_CONFIG_CMD
      // Only log a warning, do not exit code
      if (p_data[3] != 0x00) {
        STLOG_HAL_D("%s - Error in custom file, continue anyway", __func__);
      }

      break;

    case 0x60:  //
      if (p_data[1] == 0x0) {
        if (p_data[3] == 0xa0) {
          mFWInfo->hibernate_exited = 1;
        }
        if (!HalSendDownstream(mHalHandle, coreInitCmd, sizeof(coreInitCmd))) {
          STLOG_HAL_E("%s - SendDownstream failed", __func__);
        }

      } else if ((p_data[1] == 0x6) && mCustomParamDone) {
        mCustomParamDone = false;
        mGetCustomerField = false;
        hal_wrapper_update_complete();
      }
      break;
  }
}

void ApplyUwbParamHandler(HALHANDLE mHalHandle, uint16_t data_len,
                          uint8_t *p_data) {
  STLOG_HAL_D("%s - Enter ", __func__);
  if (data_len < 3) {
    STLOG_HAL_E("%s : Error, too short data (%d)", __func__, data_len);
    return;
  }

  switch (p_data[0]) {
    case 0x40:  //
      // CORE_RESET_RSP
      if ((p_data[1] == 0x0) && (p_data[3] == 0x0)) {
        // do nothing
      } else if ((p_data[1] == 0x1) && (p_data[3] == 0x0)) {
        if (mFWInfo->hibernate_exited == 0) {
          // Send a NFC mode on .
          if (!HalSendDownstream(mHalHandle, propNfcModeSetCmdOn,
                                 sizeof(propNfcModeSetCmdOn))) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }
          // CORE_INIT_RSP
        } else if ((mFWInfo->hibernate_exited == 1) && !mUwbConfigDone) {
          if (!HalSendDownstream(mHalHandle, nciPropSetUwbConfig,
                                 nciPropSetUwbConfig[2] + 3)) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }
        }

      } else {
        STLOG_HAL_D("%s - Error in uwb param application", __func__);
        I2cResetPulse();
        hal_wrapper_set_state(HAL_WRAPPER_STATE_OPEN);
      }
      break;

    case 0x4f:
      if (mFWInfo->hibernate_exited == 1) {
        if (!mUwbConfigDone) {
          mUwbConfigDone = true;
          // Check if an error has occurred for PROP_SET_CONFIG_CMD
          // Only log a warning, do not exit code
          if (p_data[3] != 0x00) {
            STLOG_HAL_D("%s - Error in uwb file, continue anyway", __func__);
          }
          if (!HalSendDownstream(mHalHandle, nciGetPropConfig,
                                 sizeof(nciGetPropConfig))) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }
        } else if ((p_data[1] == 0x2) && (p_data[2] == 0x0c)) {
          memset(nciPropSetConfig_CustomField, 0x0,
                 sizeof(nciPropSetConfig_CustomField));
          memcpy(nciPropSetConfig_CustomField, nciSetPropConfig, 9);
          nciPropSetConfig_CustomField[8] = p_data[6];
          nciPropSetConfig_CustomField[2] = p_data[6] + 6;
          memcpy(nciPropSetConfig_CustomField + 9, p_data + 7, p_data[6]);
          nciPropSetConfig_CustomField[13] = mFWInfo->uwbVersion >> 8;
          nciPropSetConfig_CustomField[14] = mFWInfo->uwbVersion;

          if (!HalSendDownstream(mHalHandle, nciPropSetConfig_CustomField,
                                 nciPropSetConfig_CustomField[2] + 3)) {
            STLOG_HAL_E("%s - SendDownstream failed", __func__);
          }

        } else {
          I2cResetPulse();
        }
      }

      break;

    case 0x60:  //
      if (p_data[1] == 0x0) {
        if (p_data[3] == 0xa0) {
          mFWInfo->hibernate_exited = 1;
        }
        if (!HalSendDownstream(mHalHandle, coreInitCmd, sizeof(coreInitCmd))) {
          STLOG_HAL_E("%s - SendDownstream failed", __func__);
        }

      } else if ((p_data[1] == 0x6) && mUwbConfigDone) {
        mUwbConfigNeeded = false;
        mUwbConfigDone = false;
        hal_wrapper_update_complete();
      }
      break;
  }
}

void SendExitLoadMode(HALHANDLE mmHalHandle) {
  STLOG_HAL_D("%s - Send APDU_EXIT_LOAD_MODE_CMD", __func__);

  if (!HalSendDownstreamTimer(mmHalHandle, ApduExitLoadMode,
                              sizeof(ApduExitLoadMode), FW_TIMER_DURATION)) {
    STLOG_HAL_E("%s - SendDownstream failed", __func__);
  }
  mHalFDState = HAL_FD_STATE_EXIT_APDU;
}
