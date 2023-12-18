/******************************************************************************
 *
 *  Copyright 2018-2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
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

/*
 * DAL spi port implementation for linux
 *
 * Project: Trusted ESE Linux
 *
 */

#include "EseSpiTransport.h"

#define LOG_TAG "NxpEseHal"
#include <log/log.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <ese_config.h>
#include <hardware/nfc.h>
#include <phEseStatus.h>
#include <phNxpEsePal.h>
#include <string.h>
#include "NfcAdaptation.h"
#include "hal_nxpese.h"
#include "phNxpEse_Api.h"

#define MAX_RETRY_CNT 10
#define HAL_NFC_SPI_DWP_SYNC 21

extern int omapi_status;

static int rf_status;
#if (NFC_NXP_ESE_VER == JCOP_VER_5_x)
eseIoctlData_t eseioctldata;
#endif
// Default max retry count for SPI CLT write blocked in secs
static unsigned long int gsMaxSpiWriteRetryCnt = 10;
#if (NFC_NXP_ESE_VER == JCOP_VER_4_0)
static ESESTATUS phNxpEse_spiIoctl_legacy(uint64_t ioctlType, void* p_data);
#endif

/*******************************************************************************
**
** Function         phPalEse_spi_close
**
** Description      Closes PN547 device
**
** Parameters       pDevHandle - device handle
**
** Returns          None
**
*******************************************************************************/
void EseSpiTransport::Close(void* pDevHandle) {
  if (NULL != pDevHandle) {
    close((intptr_t)pDevHandle);
  }
  return;
}

/*******************************************************************************
**
** Function         phNxpEse_spiIoctl
**
** Description      Perform cross HAL IOCTL functionality
**
** Parameters       ioctlType, input data
**
** Returns          SUCCESS/FAIL
**
*******************************************************************************/
ESESTATUS phNxpEse_spiIoctl(uint64_t ioctlType, void* p_data) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  if (!p_data) {
    ALOGE("halimpl phNxpEse_spiIoctl p_data is null ioctltyp: %ld",
          (long)ioctlType);
    return ESESTATUS_FAILED;
  }
#if (NFC_NXP_ESE_VER == JCOP_VER_5_x)
  ese_nxp_IoctlInOutData_t* inpOutData = (ese_nxp_IoctlInOutData_t*)p_data;
  switch (ioctlType) {
    case HAL_ESE_IOCTL_RF_STATUS_UPDATE:
      rf_status = inpOutData->inp.data.nxpCmd.p_cmd[0];
      if (rf_status == 1) {
        ALOGD_IF(
            ese_debug_enabled,
            "******************RF IS ON*************************************");
      } else {
        ALOGD_IF(
            ese_debug_enabled,
            "******************RF IS OFF*************************************");
      }
      break;
    case HAL_ESE_IOCTL_NFC_JCOP_DWNLD:

      eseioctldata.nfc_jcop_download_state =
          inpOutData->inp.data.nxpCmd.p_cmd[0];
      if (eseioctldata.nfc_jcop_download_state == 1) {
        ALOGD_IF(ese_debug_enabled,
                 "******************JCOP Download "
                 "started*************************************");
      } else {
        ALOGD_IF(ese_debug_enabled,
                 "******************JCOP Download "
                 "stopped*************************************");
      }
      break;
    default:
      ALOGD_IF(ese_debug_enabled, "Invalid IOCTL type");
      break;
  }
#endif
#if (NFC_NXP_ESE_VER == JCOP_VER_4_0)
  status = phNxpEse_spiIoctl_legacy(ioctlType, p_data);
#endif
  return status;
}
#if (NFC_NXP_ESE_VER == JCOP_VER_4_0)
/*******************************************************************************
**
** Function         phNxpEse_spiIoctl_legacy
**
** Description      Perform cross HAL IOCTL functionality
**
** Parameters       ioctlType, input data
**
** Returns          SUCCESS/FAIL
**
*******************************************************************************/
static ESESTATUS phNxpEse_spiIoctl_legacy(uint64_t ioctlType, void* p_data) {
  ese_nxp_IoctlInOutData_t* inpOutData = (ese_nxp_IoctlInOutData_t*)p_data;
  switch (ioctlType) {
    case HAL_ESE_IOCTL_RF_STATUS_UPDATE:

      rf_status = inpOutData->inp.data.nxpCmd.p_cmd[0];
      if (rf_status == 1) {
        ALOGD_IF(
            ese_debug_enabled,
            "******************RF IS ON*************************************");
      } else {
        ALOGD_IF(
            ese_debug_enabled,
            "******************RF IS OFF*************************************");
      }
      break;
    default:
      ALOGD_IF(ese_debug_enabled, "Invalid IOCTL type");
      break;
  }
  return ESESTATUS_SUCCESS;
}
#endif

/*******************************************************************************
**
** Function         OpenAndConfigure
**
** Description      Open and configure pn547 device
**
** Parameters       pConfig     - hardware information
**                  pLinkHandle - device handle
**
** Returns          ESE status:
**                  ESESTATUS_SUCCESS            - open_and_configure operation
*success
**                  ESESTATUS_INVALID_DEVICE     - device open operation failure
**
*******************************************************************************/
ESESTATUS EseSpiTransport::OpenAndConfigure(pphPalEse_Config_t pConfig) {
  int nHandle;
  int retryCnt = 0;
  ALOGD("NxpEse EseSpiTransport::OpenAndConfigure 1");
  if (EseConfig::hasKey(NAME_NXP_SOF_WRITE)) {
    mConfigSofWrite = EseConfig::getUnsigned(NAME_NXP_SOF_WRITE);
    ALOGD_IF(ese_debug_enabled, "NXP_SOF_WRITE value from config file = %ld",
             mConfigSofWrite);
  }
  if (EseConfig::hasKey(NAME_NXP_SPI_WRITE_TIMEOUT)) {
    mConfigSpiWriteTimeout = EseConfig::getUnsigned(NAME_NXP_SPI_WRITE_TIMEOUT);
    ALOGD_IF(ese_debug_enabled,
             "NXP_SPI_WRITE_TIMEOUT value from config file = %ld",
             mConfigSpiWriteTimeout);
  }
  /* Read eSE cold reset interface from ese config file */
  if (EseConfig::hasKey(NAME_NXP_P61_COLD_RESET_INTERFACE)) {
    mConfigColdResetIntf =
        EseConfig::getUnsigned(NAME_NXP_P61_COLD_RESET_INTERFACE);
    ALOGD_IF(ese_debug_enabled,
             "mConfigColdResetIntf value from config file = %ld",
             mConfigColdResetIntf);
  } else {
    mConfigColdResetIntf = 0x01; /* Default interface is NFC HAL */
    ALOGD_IF(ese_debug_enabled, "mConfigColdResetIntf: Default value ");
  }
  ALOGD_IF(ese_debug_enabled, "Opening port=%s\n", pConfig->pDevName);
/* open port */
retry:
  nHandle = open((char const*)pConfig->pDevName, O_RDWR);
  if (nHandle < 0) {
    ALOGE("%s : failed errno = 0x%x, retval %x", __FUNCTION__, errno, nHandle);

    if ((errno == -EBUSY) || (errno == EBUSY)) {
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        phPalEse_sleep(100 * 1000);  // 100ms delay
        return ESESTATUS_DRIVER_BUSY;
      } else {
        retryCnt++;
        ALOGE("Retry open eSE driver, retry cnt : %d", retryCnt);
        if (retryCnt < MAX_RETRY_CNT) {
          phPalEse_sleep(1000000);
          goto retry;
        }
      }
    }
    ALOGE("_spi_open() Failed: retval %x", nHandle);
    pConfig->pDevHandle = NULL;
    return ESESTATUS_INVALID_DEVICE;
  }
  ALOGD_IF(ese_debug_enabled, "eSE driver opened :: fd = [%d]", nHandle);
  pConfig->pDevHandle = (void*)((intptr_t)nHandle);
  return ESESTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         Read
**
** Description      Reads requested number of bytes from pn547 device into given
*buffer
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - buffer for read data
**                  nNbBytesToRead   - number of bytes requested to be read
**
** Returns          numRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*******************************************************************************/
int EseSpiTransport::Read(void* pDevHandle, uint8_t* pBuffer,
                          int nNbBytesToRead) {
  int ret = -1;
  ret = read((intptr_t)pDevHandle, (void*)pBuffer, (nNbBytesToRead));
  return ret;
}

/*******************************************************************************
**
** Function         Write
**
** Description      Writes requested number of bytes from given buffer into
*pn547 device
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - buffer for read data
**                  nNbBytesToWrite  - number of bytes requested to be written
**
** Returns          numWrote   - number of successfully written bytes
**                  -1         - write operation failure
**
*******************************************************************************/
int EseSpiTransport::Write(void* pDevHandle, uint8_t* pBuffer,
                           int nNbBytesToWrite) {
  int ret = -1;
  int numWrote = 0;
  unsigned long int retryCount = 0;
  if (NULL == pDevHandle) {
    ALOGE("phPalEse_spi_write: received pDevHandle=NULL");
    return -1;
  }
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    if (mConfigSofWrite == 1) {
      /* Appending SOF for SPI write */
      pBuffer[0] = SEND_PACKET_SOF;
    } else {
      /* Do Nothing */
    }
  }
  ALOGE("NXP_SPI_WRITE_TIMEOUT value is... : %ld secs", mConfigSpiWriteTimeout);
  if (mConfigSpiWriteTimeout > 0) {
    gsMaxSpiWriteRetryCnt = mConfigSpiWriteTimeout;
    ALOGE(" spi_write_timeout Wait time ... : %ld", gsMaxSpiWriteRetryCnt);
  } else {
    /* Do Nothing */
  }

  while (numWrote < nNbBytesToWrite) {
    // usleep(5000);
    if (rf_status == 0) {
      ret = write((intptr_t)pDevHandle, pBuffer + numWrote,
                  nNbBytesToWrite - numWrote);
    } else {
      ret = -1;
    }
    if (ret > 0) {
      numWrote += ret;
    } else if (ret == 0) {
      ALOGE("_spi_write() EOF");
      return -1;
    } else {
      ALOGE("_spi_write() errno : %x", errno);
      ALOGD_IF(ese_debug_enabled, "rf_status value is %d", rf_status);
      if ((errno == EINTR || errno == EAGAIN || rf_status == 1) &&
          (retryCount < gsMaxSpiWriteRetryCnt)) {
        /*Configure retry count or timeout here,now its configured for 2*10
         * secs*/
        if (retryCount > gsMaxSpiWriteRetryCnt) {
          ret = -1;
          break;
        }

        retryCount++;
        /* 5ms delay to give ESE wake up delay */
        phPalEse_sleep(1000 * (GET_WAKE_UP_DELAY()));
        ALOGE("_spi_write() failed. Going to retry, counter:%ld !", retryCount);
        continue;
      }
      return -1;
    }
  }
  return numWrote;
}

/*******************************************************************************
**
** Function         Ioctl
**
** Description      Exposed ioctl by p61 spi driver
**
** Parameters       pDevHandle     - valid device handle
**                  level          - reset level
**
** Returns           0   - ioctl operation success
**                  -1   - ioctl operation failure
**
*******************************************************************************/
ESESTATUS EseSpiTransport::Ioctl(phPalEse_ControlCode_t eControlCode,
                                 void* pDevHandle, long level) {
  ESESTATUS ret = ESESTATUS_IOCTL_FAILED;
  int retioctl = 0x00;
#if (NFC_NXP_ESE_VER == JCOP_VER_5_x)
  ese_nxp_IoctlInOutData_t inpOutData;
  inpOutData.inp.level = level;
  NfcAdaptation& pNfcAdapt = NfcAdaptation::GetInstance();
#endif
  ALOGD_IF(ese_debug_enabled, "phPalEse_spi_ioctl(), ioctl %x , level %lx",
           eControlCode, level);
  if (NULL == pDevHandle) {
    if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
      return ESESTATUS_IOCTL_FAILED;
    }
  }
  switch (eControlCode) {
    case phPalEse_e_ResetDevice:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_PWR, level);
      }
      break;

    case phPalEse_e_EnableLog:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_DBG, level);
      }
      break;

    case phPalEse_e_EnablePollMode:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_POLL, level);
      }
      break;
    case phPalEse_e_SetSecureMode:
      ret =
          (ESESTATUS)ioctl((intptr_t)pDevHandle, ESE_SET_TRUSTED_ACCESS, level);
      if (0x00 <= ret) {
        ret = ESESTATUS_SUCCESS;
      }
      break;
    case phPalEse_e_ChipRst:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        if (level == 5) {              // SPI driver communication part
          if (!mConfigColdResetIntf) { /* Call the driver IOCTL */
            retioctl =
                ioctl((intptr_t)pDevHandle, ESE_PERFORM_COLD_RESET, level);
            if (0x00 <= retioctl) {
              ret = ESESTATUS_SUCCESS;
            }
          } else {
#if (NFC_NXP_ESE_VER == JCOP_VER_5_x)
            // Nfc Driver communication part
            ret = pNfcAdapt.resetEse(level);
#else
            ret = ESESTATUS_SUCCESS;
#endif
          }
        } else {
          ret = ESESTATUS_SUCCESS;
        }
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_SPM_PWR, level);
      }
      break;
    case phPalEse_e_ResetProtection:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        retioctl = ioctl((intptr_t)pDevHandle, PERFORM_RESET_PROTECTION, level);
        if (0x00 <= retioctl) {
          ret = ESESTATUS_SUCCESS;
        } else {
          ALOGE("phPalEse_e_ResetProtection ioctl failed status :%x !",
                retioctl);
        }
      }
      break;
    case phPalEse_e_EnableThroughputMeasurement:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_THROUGHPUT, level);
      }
      break;

    case phPalEse_e_SetPowerScheme:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret =
            (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_POWER_SCHEME, level);
      }
      break;

    case phPalEse_e_GetSPMStatus:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_GET_SPM_STATUS, level);
      }
      break;

    case phPalEse_e_GetEseAccess:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret = (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_GET_ESE_ACCESS, level);
      }
      break;
    case phPalEse_e_SetJcopDwnldState:
      if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
        ret = ESESTATUS_SUCCESS;
      } else {
        ret =
            (ESESTATUS)ioctl((intptr_t)pDevHandle, P61_SET_DWNLD_STATUS, level);
      }
      break;
#if (NFC_NXP_ESE_VER == JCOP_VER_5_x)
    case phPalEse_e_SetClientUpdateState: {
      pNfcAdapt.Initialize();
      ALOGD_IF(ese_debug_enabled,
               "phPalEse_spi_ioctl state = phPalEse_e_SetJcopDwnldState");
      ese_nxp_IoctlInOutData_t inpOutData;
      memset(&inpOutData, 0x00, sizeof(ese_nxp_IoctlInOutData_t));
      inpOutData.inp.data.nxpCmd.cmd_len = 1;
      inpOutData.inp.data_source = 1;
      uint8_t data = (uint8_t)level;
      memcpy(inpOutData.inp.data.nxpCmd.p_cmd, &data, sizeof(data));
      ALOGD_IF(ese_debug_enabled, "Before phPalEse_e_SetClientUpdateState");

      ret = pNfcAdapt.setEseUpdateState(&inpOutData);
      ALOGD_IF(ese_debug_enabled, "After phPalEse_e_SetClientUpdateState");
    } break;
#endif
    case phPalEse_e_DisablePwrCntrl:
      ret = ESESTATUS_SUCCESS;
      break;
    default:
      ret = ESESTATUS_IOCTL_FAILED;
      break;
  }
  ALOGD_IF(ese_debug_enabled, "Exit  phPalEse_spi_ioctl : ret = %d errno = %d",
           ret, errno);
  return (ESESTATUS)ret;
}
