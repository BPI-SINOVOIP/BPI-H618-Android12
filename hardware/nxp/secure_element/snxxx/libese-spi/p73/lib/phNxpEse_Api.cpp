/******************************************************************************
 *
 *  Copyright 2018-2021 NXP
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
#define LOG_TAG "NxpEseHal"
#include <log/log.h>

#include <EseTransport.h>
#include <cutils/properties.h>
#include <ese_config.h>
#include <phNxpEseFeatures.h>
#include <phNxpEsePal.h>
#include <phNxpEseProto7816_3.h>
#include <phNxpEse_Internal.h>

#define RECEIVE_PACKET_SOF 0xA5
#define CHAINED_PACKET_WITHSEQN 0x60
#define CHAINED_PACKET_WITHOUTSEQN 0x20
#define PH_PAL_ESE_PRINT_PACKET_TX(data, len) \
  ({ phPalEse_print_packet("SEND", data, len); })
#define PH_PAL_ESE_PRINT_PACKET_RX(data, len) \
  ({ phPalEse_print_packet("RECV", data, len); })
/* 32K(0x8000) Datasize + 10(0xA) Byte Max Header Size + 1 byte negative
 * testcase support */
#define MAX_SUPPORTED_DATA_SIZE 0x800B
static int phNxpEse_readPacket(void* pDevHandle, uint8_t* pBuffer,
                               int nNbBytesToRead);
static int phNxpEse_readPacket_legacy(void* pDevHandle, uint8_t* pBuffer,
                                      int nNbBytesToRead);

static ESESTATUS phNxpEse_checkJcopDwnldState(void);
static ESESTATUS phNxpEse_setJcopDwnldState(phNxpEse_JcopDwnldState state);
static ESESTATUS phNxpEse_checkFWDwnldStatus(void);
static void phNxpEse_GetMaxTimer(unsigned long* pMaxTimer);
static unsigned char* phNxpEse_GgetTimerTlvBuffer(unsigned char* timer_buffer,
                                                  unsigned int value);
static __inline bool phNxpEse_isColdResetRequired(phNxpEse_initMode mode,
                                                  ESESTATUS status);
static int poll_sof_chained_delay = 0;
static phNxpEse_OsVersion_t sOsVersion = INVALID_OS_VERSION;
/* To Overwrite the value of wtx_counter_limit from config file*/
static unsigned long int app_wtx_cnt = RESET_APP_WTX_COUNT;

/*********************** Global Variables *************************************/

/* ESE Context structure */
phNxpEse_Context_t nxpese_ctxt;
bool ese_debug_enabled = false;

/******************************************************************************
 * Function         phNxpEse_SetEndPoint_Cntxt
 *
 * Description      This function is called set the SE endpoint
 *
 * Returns          None
 *
 ******************************************************************************/

ESESTATUS phNxpEse_SetEndPoint_Cntxt(uint8_t uEndPoint) {
  ESESTATUS status = ESESTATUS_FAILED;
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    status = phNxpEseProto7816_SetEndPoint(uEndPoint);
    if (status == ESESTATUS_SUCCESS) {
      nxpese_ctxt.nadInfo.nadRx = nadInfoRx_ptr[uEndPoint];
      nxpese_ctxt.nadInfo.nadTx = nadInfoTx_ptr[uEndPoint];
      nxpese_ctxt.endPointInfo = uEndPoint;
    }
    ALOGD_IF(ese_debug_enabled, "%s: Enpoint=%d", __FUNCTION__, uEndPoint);
  } else {
    ALOGE("%s- Function not supported", __FUNCTION__);
  }
  return status;
}

/******************************************************************************
 * Function         phNxpEse_ResetEndPoint_Cntxt
 *
 * Description      This function is called to reset the SE endpoint
 *
 * Returns          None
 *
 ******************************************************************************/
ESESTATUS phNxpEse_ResetEndPoint_Cntxt(uint8_t uEndPoint) {
  ESESTATUS status = ESESTATUS_FAILED;
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    status = phNxpEseProto7816_ResetEndPoint(uEndPoint);
  } else {
    ALOGE("%s- Function not supported", __FUNCTION__);
  }
  return status;
}
/******************************************************************************
 * Function         phNxpLog_InitializeLogLevel
 *
 * Description      This function is called during phNxpEse_init to initialize
 *                  debug log level.
 *
 * Returns          None
 *
 ******************************************************************************/

void phNxpLog_InitializeLogLevel() {
  ese_debug_enabled =
      (EseConfig::getUnsigned(NAME_SE_DEBUG_ENABLED, 0) != 0) ? true : false;

  char valueStr[PROPERTY_VALUE_MAX] = {0};
  int len = property_get("vendor.ese.debug_enabled", valueStr, "");
  if (len > 0) {
    // let Android property override .conf variable
    unsigned debug_enabled = 0;
    sscanf(valueStr, "%u", &debug_enabled);
    ese_debug_enabled = (debug_enabled == 0) ? false : true;
  }

  ALOGD_IF(ese_debug_enabled, "%s: level=%u", __func__, ese_debug_enabled);
}

/******************************************************************************
 * Function         phNxpEse_init
 *
 * Description      This function is called by Jni/phNxpEse_open during the
 *                  initialization of the ESE. It initializes protocol stack
 *instance variable
 *
 * Returns          This function return ESESTATUS_SUCCESS (0) in case of
 *success In case of failure returns other failure value.
 *
 ******************************************************************************/
ESESTATUS phNxpEse_init(phNxpEse_initParams initParams) {
  ESESTATUS wConfigStatus = ESESTATUS_FAILED;
  unsigned long int num, ifsd_value = 0;
  unsigned long maxTimer = 0;
  uint8_t retry = 0;
  phNxpEseProto7816InitParam_t protoInitParam;
  phNxpEse_memset(&protoInitParam, 0x00, sizeof(phNxpEseProto7816InitParam_t));
  /* STATUS_OPEN */
  nxpese_ctxt.EseLibStatus = ESE_STATUS_OPEN;

  if (app_wtx_cnt > RESET_APP_WTX_COUNT) {
    protoInitParam.wtx_counter_limit = app_wtx_cnt;
    ALOGD_IF(ese_debug_enabled, "Wtx_counter limit from app setting - %lu",
             protoInitParam.wtx_counter_limit);
  } else {
    protoInitParam.wtx_counter_limit = EseConfig::getUnsigned(
        NAME_NXP_WTX_COUNT_VALUE, PH_PROTO_WTX_DEFAULT_COUNT);
    ALOGD_IF(ese_debug_enabled, "Wtx_counter read from config file - %lu",
             protoInitParam.wtx_counter_limit);
  }
  if (EseConfig::hasKey(NAME_RNACK_RETRY_DELAY)) {
    num = EseConfig::getUnsigned(NAME_RNACK_RETRY_DELAY);
    nxpese_ctxt.invalidFrame_Rnack_Delay = num;
    ALOGD_IF(ese_debug_enabled, "Rnack retry_delay read from config file - %lu",
             num);
  } else {
    nxpese_ctxt.invalidFrame_Rnack_Delay = 7000;
  }
  if (EseConfig::hasKey(NAME_NXP_MAX_RNACK_RETRY)) {
    protoInitParam.rnack_retry_limit =
        EseConfig::getUnsigned(NAME_NXP_MAX_RNACK_RETRY);
  } else {
    protoInitParam.rnack_retry_limit = MAX_RNACK_RETRY_LIMIT;
  }
  if (ESE_MODE_NORMAL ==
      initParams.initMode) /* TZ/Normal wired mode should come here*/
  {
    if (EseConfig::hasKey(NAME_NXP_SPI_INTF_RST_ENABLE)) {
      protoInitParam.interfaceReset =
          (EseConfig::getUnsigned(NAME_NXP_SPI_INTF_RST_ENABLE) == 1) ? true
                                                                      : false;
    } else {
      protoInitParam.interfaceReset = true;
    }
  } else /* OSU mode, no interface reset is required */
  {
    if (phNxpEse_doResetProtection(true)) {
      ALOGE("%s Reset Potection failed. returning...", __FUNCTION__);
      return ESESTATUS_FAILED;
    }
    protoInitParam.interfaceReset = false;
  }
  if (EseConfig::hasKey(NAME_NXP_WTX_NTF_COUNT)) {
    num = EseConfig::getUnsigned(NAME_NXP_WTX_NTF_COUNT);
    protoInitParam.wtx_ntf_limit = num;
    ALOGD_IF(ese_debug_enabled, "Wtx_ntf limit from config file - %lu",
             protoInitParam.wtx_ntf_limit);
  } else {
    protoInitParam.wtx_ntf_limit = PH_DEFAULT_WTX_NTF_LIMIT;
  }
  nxpese_ctxt.fPtr_WtxNtf = initParams.fPtr_WtxNtf;
  /* Sharing lib context for fetching secure timer values */
  protoInitParam.pSecureTimerParams =
      (phNxpEseProto7816SecureTimer_t*)&nxpese_ctxt.secureTimerParams;

  ALOGD_IF(ese_debug_enabled,
           "%s secureTimer1 0x%x secureTimer2 0x%x secureTimer3 0x%x",
           __FUNCTION__, nxpese_ctxt.secureTimerParams.secureTimer1,
           nxpese_ctxt.secureTimerParams.secureTimer2,
           nxpese_ctxt.secureTimerParams.secureTimer3);

  phNxpEse_GetMaxTimer(&maxTimer);
#ifdef SPM_INTEGRATED
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    wConfigStatus = phNxpEse_SPM_DisablePwrControl(maxTimer);
    if (wConfigStatus != ESESTATUS_SUCCESS) {
      ALOGE("%s phNxpEse_SPM_DisablePwrControl: failed", __FUNCTION__);
    }
  }
#endif
  do {
    /* T=1 Protocol layer open */
    wConfigStatus = phNxpEseProto7816_Open(protoInitParam);
    if (phNxpEse_isColdResetRequired(initParams.initMode, wConfigStatus))
      phNxpEse_SPM_ConfigPwr(SPM_RECOVERY_RESET);
  } while (phNxpEse_isColdResetRequired(initParams.initMode, wConfigStatus) &&
           retry++ < 1);
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    if (ESESTATUS_TRANSCEIVE_FAILED == wConfigStatus ||
        ESESTATUS_FAILED == wConfigStatus) {
      nxpese_ctxt.EseLibStatus = ESE_STATUS_RECOVERY;
    }
  }

  if (ESESTATUS_SUCCESS == wConfigStatus) {
    ALOGD_IF(ese_debug_enabled, "phNxpEseProto7816_Open completed >>>>>");
    /* Retrieving the IFS-D value configured in the config file and applying to
     * Card */
    if ((nxpese_ctxt.endPointInfo == END_POINT_ESE) &&
        (EseConfig::hasKey(NAME_NXP_ESE_IFSD_VALUE))) {
      ifsd_value = EseConfig::getUnsigned(NAME_NXP_ESE_IFSD_VALUE);
      if ((0xFFFF > ifsd_value) && (ifsd_value > 0)) {
        ALOGD_IF(ese_debug_enabled,
                 "phNxpEseProto7816_SetIFS IFS adjustment requested with %ld",
                 ifsd_value);
        phNxpEse_setIfs(ifsd_value);
      } else {
        ALOGD_IF(ese_debug_enabled,
                 "phNxpEseProto7816_SetIFS IFS adjustment argument invalid");
      }
    } else if ((nxpese_ctxt.endPointInfo == END_POINT_EUICC) &&
               (EseConfig::hasKey(NAME_NXP_EUICC_IFSD_VALUE))) {
      ifsd_value = EseConfig::getUnsigned(NAME_NXP_EUICC_IFSD_VALUE);
      if ((0xFFFF > ifsd_value) && (ifsd_value > 0)) {
        ALOGD_IF(ese_debug_enabled,
                 "phNxpEseProto7816_SetIFS IFS adjustment requested with %ld",
                 ifsd_value);
        phNxpEse_setIfs(ifsd_value);
      } else {
        ALOGD_IF(ese_debug_enabled,
                 "phNxpEseProto7816_SetIFS IFS adjustment argument invalid");
      }
    }
  } else {
    ALOGE("phNxpEseProto7816_Open failed with status = %x", wConfigStatus);
  }

  return wConfigStatus;
}

/******************************************************************************
 * Function         phNxpEse_open
 *
 * Description      This function is called by Jni during the
 *                  initialization of the ESE. It opens the physical connection
 *                  with ESE and creates required NAME_NXP_MAX_RNACK_RETRYclient
 *                  thread for operation.
 * Returns          This function return ESESTATUS_SUCCESS (0) in case of
 *                  success. In case of failure returns other failure values.
 ******************************************************************************/
ESESTATUS phNxpEse_open(phNxpEse_initParams initParams) {
  phPalEse_Config_t tPalConfig;
  ESESTATUS wConfigStatus = ESESTATUS_SUCCESS;
  unsigned long int num = 0, tpm_enable = 0;
  char ese_dev_node[64];
  std::string ese_node;
#ifdef SPM_INTEGRATED
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  spm_state_t current_spm_state = SPM_STATE_INVALID;
#endif
  /* initialize trace level */
  phNxpLog_InitializeLogLevel();

  ALOGD_IF(ese_debug_enabled, "phNxpEse_open Enter");
  /*When spi channel is already opened return status as FAILED*/
  if (nxpese_ctxt.EseLibStatus != ESE_STATUS_CLOSE) {
    ALOGD_IF(ese_debug_enabled, "already opened\n");
    return ESESTATUS_BUSY;
  }

  phNxpEse_memset(&nxpese_ctxt, 0x00, sizeof(nxpese_ctxt));
  phNxpEse_memset(&tPalConfig, 0x00, sizeof(tPalConfig));

  ALOGD_IF(ese_debug_enabled, "MW SEAccessKit Version");
  ALOGD_IF(ese_debug_enabled, "Android Version:0x%x", NXP_ANDROID_VER);
  ALOGD_IF(ese_debug_enabled, "Major Version:0x%x", ESELIB_MW_VERSION_MAJ);
  ALOGD_IF(ese_debug_enabled, "Minor Version:0x%x", ESELIB_MW_VERSION_MIN);

  if (EseConfig::hasKey(NAME_NXP_OS_VERSION)) {
    num = EseConfig::getUnsigned(NAME_NXP_OS_VERSION);
    ALOGD_IF(ese_debug_enabled, "Chip type read from config file - %lu", num);
    sOsVersion = (num == 1) ? OS_VERSION_4_0
                            : ((num == 2) ? OS_VERSION_5_1 : OS_VERSION_5_2);
  } else {
    sOsVersion = OS_VERSION_5_2;
    ALOGD_IF(ese_debug_enabled,
             "Chip type not defined in config file osVersion- %d", sOsVersion);
  }
  if (EseConfig::hasKey(NAME_NXP_TP_MEASUREMENT)) {
    tpm_enable = EseConfig::getUnsigned(NAME_NXP_TP_MEASUREMENT);
    ALOGD_IF(
        ese_debug_enabled,
        "SPI Throughput measurement enable/disable read from config file - %lu",
        tpm_enable);
  } else {
    ALOGD_IF(ese_debug_enabled,
             "SPI Throughput not defined in config file - %lu", tpm_enable);
  }
#if (NXP_POWER_SCHEME_SUPPORT == true)
  if (EseConfig::hasKey(NAME_NXP_POWER_SCHEME)) {
    num = EseConfig::getUnsigned(NAME_NXP_POWER_SCHEME);
    nxpese_ctxt.pwr_scheme = num;
    ALOGD_IF(ese_debug_enabled, "Power scheme read from config file - %lu",
             num);
  } else {
    nxpese_ctxt.pwr_scheme = PN67T_POWER_SCHEME;
    ALOGD_IF(ese_debug_enabled, "Power scheme not defined in config file - %lu",
             num);
  }
#else
  nxpese_ctxt.pwr_scheme = PN67T_POWER_SCHEME;
  tpm_enable = 0x00;
#endif

  if (EseConfig::hasKey(NAME_NXP_NAD_POLL_RETRY_TIME)) {
    num = EseConfig::getUnsigned(NAME_NXP_NAD_POLL_RETRY_TIME);
    nxpese_ctxt.nadPollingRetryTime = num;
  } else {
    nxpese_ctxt.nadPollingRetryTime = 5;
  }

  ALOGD_IF(ese_debug_enabled, "Nad poll retry time in us - %lu us",
           nxpese_ctxt.nadPollingRetryTime * GET_WAKE_UP_DELAY() *
               NAD_POLLING_SCALER);

  /*Read device node path*/
  ese_node = EseConfig::getString(NAME_NXP_ESE_DEV_NODE, "/dev/pn81a");
  strlcpy(ese_dev_node, ese_node.c_str(), sizeof(ese_dev_node));
  tPalConfig.pDevName = (int8_t*)ese_dev_node;

  /* Initialize PAL layer */
  wConfigStatus = phPalEse_open_and_configure(&tPalConfig);
  if (wConfigStatus != ESESTATUS_SUCCESS) {
    ALOGE("phPalEse_Init Failed");
    if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
      if (ESESTATUS_DRIVER_BUSY == wConfigStatus)
        ALOGE("Ese Driver is Busy!!!");
    }
    goto clean_and_return;
  }
  /* Copying device handle to ESE Lib context*/
  nxpese_ctxt.pDevHandle = tPalConfig.pDevHandle;
  if (ESE_PROTOCOL_MEDIA_SPI == initParams.mediaType) {
    ALOGD_IF(ese_debug_enabled,
             "Inform eSE about the starting of trusted Mode");
    wConfigStatus =
        phPalEse_ioctl(phPalEse_e_SetSecureMode, tPalConfig.pDevHandle, 0x01);
    if (ESESTATUS_SUCCESS != wConfigStatus) goto clean_and_return_2;
  }
#ifdef SPM_INTEGRATED
  /* Get the Access of ESE*/
  wSpmStatus = phNxpEse_SPM_Init(nxpese_ctxt.pDevHandle);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_Init Failed");
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_2;
  }
  wSpmStatus = phNxpEse_SPM_SetPwrScheme(nxpese_ctxt.pwr_scheme);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE(" %s : phNxpEse_SPM_SetPwrScheme Failed", __FUNCTION__);
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_1;
  }
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    wConfigStatus = phNxpEse_checkFWDwnldStatus();
    if (wConfigStatus != ESESTATUS_SUCCESS) {
      ALOGE("Failed to open SPI due to VEN pin used by FW download \n");
      wConfigStatus = ESESTATUS_FAILED;
      goto clean_and_return_1;
    }
  }
  wSpmStatus = phNxpEse_SPM_GetState(&current_spm_state);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE(" %s : phNxpEse_SPM_GetPwrState Failed", __FUNCTION__);
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_1;
  } else {
    if (((current_spm_state & SPM_STATE_SPI) |
         (current_spm_state & SPM_STATE_SPI_PRIO)) &&
        !(current_spm_state & SPM_STATE_SPI_FAILED)) {
      ALOGE(" %s : SPI is already opened...second instance not allowed",
            __FUNCTION__);
      wConfigStatus = ESESTATUS_FAILED;
      goto clean_and_return_1;
    }
  }
  if (current_spm_state & SPM_STATE_JCOP_DWNLD) {
    ALOGE(" %s : Denying to open JCOP Download in progress", __FUNCTION__);
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_1;
  }
  phNxpEse_memcpy(&nxpese_ctxt.initParams, &initParams,
                  sizeof(phNxpEse_initParams));
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    /* Updating ESE power state based on the init mode */
    if (ESE_MODE_OSU == nxpese_ctxt.initParams.initMode) {
      ALOGD_IF(ese_debug_enabled, "%s Init mode ---->OSU", __FUNCTION__);
      wConfigStatus = phNxpEse_checkJcopDwnldState();
      if (wConfigStatus != ESESTATUS_SUCCESS) {
        ALOGE("phNxpEse_checkJcopDwnldState failed");
        goto clean_and_return_1;
      }
    }
  }
  wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_POWER_ENABLE);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_ConfigPwr: enabling power Failed");
    if (wSpmStatus == ESESTATUS_BUSY) {
      wConfigStatus = ESESTATUS_BUSY;
    } else if (wSpmStatus == ESESTATUS_DWNLD_BUSY) {
      wConfigStatus = ESESTATUS_DWNLD_BUSY;
    } else {
      wConfigStatus = ESESTATUS_FAILED;
    }
    goto clean_and_return;
  } else {
    ALOGD_IF(ese_debug_enabled, "nxpese_ctxt.spm_power_state true");
    nxpese_ctxt.spm_power_state = true;
  }
#endif
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    if (tpm_enable) {
      wConfigStatus = phPalEse_ioctl(phPalEse_e_EnableThroughputMeasurement,
                                     nxpese_ctxt.pDevHandle, 0);
      if (wConfigStatus != ESESTATUS_SUCCESS) {
        ALOGE("phPalEse_IoCtl Failed");
        goto clean_and_return;
      }
    }
  }
  ALOGD_IF(ese_debug_enabled, "wConfigStatus %x", wConfigStatus);
  return wConfigStatus;

clean_and_return:
#ifdef SPM_INTEGRATED
  wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_POWER_DISABLE);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_ConfigPwr: disabling power Failed");
  }
clean_and_return_1:
  phNxpEse_SPM_DeInit();
clean_and_return_2:
#endif
  if (NULL != nxpese_ctxt.pDevHandle) {
    phPalEse_close(nxpese_ctxt.pDevHandle);
    phNxpEse_memset(&nxpese_ctxt, 0x00, sizeof(nxpese_ctxt));
  }
  nxpese_ctxt.EseLibStatus = ESE_STATUS_CLOSE;
  nxpese_ctxt.spm_power_state = false;
  return ESESTATUS_FAILED;
}

/******************************************************************************
 * \ingroup spi_libese
 *
 * \brief  Check if libese has opened
 *
 * \retval return false if it is close, otherwise true.
 **
 ******************************************************************************/
bool phNxpEse_isOpen() { return nxpese_ctxt.EseLibStatus != ESE_STATUS_CLOSE; }

/******************************************************************************
 * Function         phNxpEse_openPrioSession
 *
 * Description      This function is called by Jni during the
 *                  initialization of the ESE. It opens the physical connection
 *                  with ESE () and creates required client thread for
 *                  operation.  This will get priority access to ESE for timeout
 duration.

 * Returns          This function return ESESTATUS_SUCCESS (0) in case of
 success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
ESESTATUS phNxpEse_openPrioSession(phNxpEse_initParams initParams) {
  phPalEse_Config_t tPalConfig;
  ESESTATUS wConfigStatus = ESESTATUS_SUCCESS;
  unsigned long int num = 0, tpm_enable = 0;

  /* initialize trace level */
  phNxpLog_InitializeLogLevel();
  ALOGD_IF(ese_debug_enabled, "phNxpEse_openPrioSession Enter");
#ifdef SPM_INTEGRATED
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  spm_state_t current_spm_state = SPM_STATE_INVALID;
#endif
  phNxpEse_memset(&nxpese_ctxt, 0x00, sizeof(nxpese_ctxt));
  phNxpEse_memset(&tPalConfig, 0x00, sizeof(tPalConfig));

  ALOGD_IF(ese_debug_enabled, "MW SEAccessKit Version");
  ALOGD_IF(ese_debug_enabled, "Android Version:0x%x", NXP_ANDROID_VER);
  ALOGD_IF(ese_debug_enabled, "Major Version:0x%x", ESELIB_MW_VERSION_MAJ);
  ALOGD_IF(ese_debug_enabled, "Minor Version:0x%x", ESELIB_MW_VERSION_MIN);

#if (NXP_POWER_SCHEME_SUPPORT == true)
  if (EseConfig::hasKey(NAME_NXP_POWER_SCHEME)) {
    num = EseConfig::getUnsigned(NAME_NXP_POWER_SCHEME);
    nxpese_ctxt.pwr_scheme = num;
    ALOGD_IF(ese_debug_enabled, "Power scheme read from config file - %lu",
             num);
  } else
#endif
  {
    nxpese_ctxt.pwr_scheme = PN67T_POWER_SCHEME;
    ALOGD_IF(ese_debug_enabled, "Power scheme not defined in config file - %lu",
             num);
  }
  if (EseConfig::hasKey(NAME_NXP_TP_MEASUREMENT)) {
    tpm_enable = EseConfig::getUnsigned(NAME_NXP_TP_MEASUREMENT);
    ALOGD_IF(
        ese_debug_enabled,
        "SPI Throughput measurement enable/disable read from config file - %lu",
        tpm_enable);
  } else {
    ALOGD_IF(ese_debug_enabled,
             "SPI Throughput not defined in config file - %lu", num);
  }

  tPalConfig.pDevName = (int8_t*)"/dev/p73";

  /* Initialize PAL layer */
  wConfigStatus = phPalEse_open_and_configure(&tPalConfig);
  if (wConfigStatus != ESESTATUS_SUCCESS) {
    ALOGE("phPalEse_Init Failed");
    goto clean_and_return;
  }
  /* Copying device handle to hal context*/
  nxpese_ctxt.pDevHandle = tPalConfig.pDevHandle;

#ifdef SPM_INTEGRATED
  /* Get the Access of ESE*/
  wSpmStatus = phNxpEse_SPM_Init(nxpese_ctxt.pDevHandle);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_Init Failed");
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_2;
  }
  wSpmStatus = phNxpEse_SPM_SetPwrScheme(nxpese_ctxt.pwr_scheme);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE(" %s : phNxpEse_SPM_SetPwrScheme Failed", __FUNCTION__);
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_1;
  }
  wSpmStatus = phNxpEse_SPM_GetState(&current_spm_state);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE(" %s : phNxpEse_SPM_GetPwrState Failed", __FUNCTION__);
    wConfigStatus = ESESTATUS_FAILED;
    goto clean_and_return_1;
  } else {
    if ((current_spm_state & SPM_STATE_SPI) |
        (current_spm_state & SPM_STATE_SPI_PRIO)) {
      ALOGE(" %s : SPI is already opened...second instance not allowed",
            __FUNCTION__);
      wConfigStatus = ESESTATUS_FAILED;
      goto clean_and_return_1;
    }
    if (current_spm_state & SPM_STATE_JCOP_DWNLD) {
      ALOGE(" %s : Denying to open JCOP Download in progress", __FUNCTION__);
      wConfigStatus = ESESTATUS_FAILED;
      goto clean_and_return_1;
    }
    if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
      wConfigStatus = phNxpEse_checkFWDwnldStatus();
      if (wConfigStatus != ESESTATUS_SUCCESS) {
        ALOGD_IF(ese_debug_enabled,
                 "Failed to open SPI due to VEN pin used by FW download \n");
        wConfigStatus = ESESTATUS_FAILED;
        goto clean_and_return_1;
      }
    }
  }
  phNxpEse_memcpy(&nxpese_ctxt.initParams, &initParams.initMode,
                  sizeof(phNxpEse_initParams));
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    /* Updating ESE power state based on the init mode */
    if (ESE_MODE_OSU == nxpese_ctxt.initParams.initMode) {
      wConfigStatus = phNxpEse_checkJcopDwnldState();
      if (wConfigStatus != ESESTATUS_SUCCESS) {
        ALOGE("phNxpEse_checkJcopDwnldState failed");
        goto clean_and_return_1;
      }
    }
  }
  wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_POWER_PRIO_ENABLE);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_ConfigPwr: enabling power for spi prio Failed");
    if (wSpmStatus == ESESTATUS_BUSY) {
      wConfigStatus = ESESTATUS_BUSY;
    } else if (wSpmStatus == ESESTATUS_DWNLD_BUSY) {
      wConfigStatus = ESESTATUS_DWNLD_BUSY;
    } else {
      wConfigStatus = ESESTATUS_FAILED;
    }
    goto clean_and_return;
  } else {
    ALOGE("nxpese_ctxt.spm_power_state true");
    nxpese_ctxt.spm_power_state = true;
  }
#endif

#ifndef SPM_INTEGRATED
  wConfigStatus =
      phPalEse_ioctl(phPalEse_e_ResetDevice, nxpese_ctxt.pDevHandle, 2);
  if (wConfigStatus != ESESTATUS_SUCCESS) {
    ALOGE("phPalEse_IoCtl Failed");
    goto clean_and_return;
  }
#endif
  wConfigStatus =
      phPalEse_ioctl(phPalEse_e_EnableLog, nxpese_ctxt.pDevHandle, 0);
  if (wConfigStatus != ESESTATUS_SUCCESS) {
    ALOGE("phPalEse_IoCtl Failed");
    goto clean_and_return;
  }
  wConfigStatus =
      phPalEse_ioctl(phPalEse_e_EnablePollMode, nxpese_ctxt.pDevHandle, 1);
  if (wConfigStatus != ESESTATUS_SUCCESS) {
    ALOGE("phPalEse_IoCtl Failed");
    goto clean_and_return;
  }
  ALOGD_IF(ese_debug_enabled, "wConfigStatus %x", wConfigStatus);
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    if (tpm_enable) {
      wConfigStatus = phPalEse_ioctl(phPalEse_e_EnableThroughputMeasurement,
                                     nxpese_ctxt.pDevHandle, 0);
      if (wConfigStatus != ESESTATUS_SUCCESS) {
        ALOGE("phPalEse_IoCtl Failed");
        goto clean_and_return;
      }
    }
  }
  return wConfigStatus;

clean_and_return:
#ifdef SPM_INTEGRATED
  wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_POWER_DISABLE);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_ConfigPwr: disabling power Failed");
  }
clean_and_return_1:
  phNxpEse_SPM_DeInit();
clean_and_return_2:
#endif
  if (NULL != nxpese_ctxt.pDevHandle) {
    phPalEse_close(nxpese_ctxt.pDevHandle);
    phNxpEse_memset(&nxpese_ctxt, 0x00, sizeof(nxpese_ctxt));
  }
  nxpese_ctxt.EseLibStatus = ESE_STATUS_CLOSE;
  nxpese_ctxt.spm_power_state = false;
  return ESESTATUS_FAILED;
}

/******************************************************************************
 * Function         phNxpEse_setJcopDwnldState
 *
 * Description      This function is  used to check whether JCOP OS
 *                  download can be started or not.
 *
 * Returns          returns  ESESTATUS_SUCCESS or ESESTATUS_FAILED
 *
 ******************************************************************************/
static ESESTATUS phNxpEse_setJcopDwnldState(phNxpEse_JcopDwnldState state) {
  ESESTATUS wConfigStatus = ESESTATUS_FAILED;
  ALOGD_IF(ese_debug_enabled, "phNxpEse_setJcopDwnldState Enter");

  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    wConfigStatus = phNxpEse_SPM_SetJcopDwnldState(state);
  } else {
    ALOGE("%s function not supported", __FUNCTION__);
  }
  return wConfigStatus;
}

/******************************************************************************
 * Function         phNxpEse_checkJcopDwnldState
 *
 * Description      This function is  used to check whether JCOP OS
 *                  download can be started or not.
 *
 * Returns          returns  ESESTATUS_SUCCESS or ESESTATUS_BUSY
 *
 ******************************************************************************/
static ESESTATUS phNxpEse_checkJcopDwnldState(void) {
  ALOGD_IF(ese_debug_enabled, "phNxpEse_checkJcopDwnld Enter");
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  spm_state_t current_spm_state = SPM_STATE_INVALID;
  uint8_t ese_dwnld_retry = 0x00;
  ESESTATUS status = ESESTATUS_FAILED;

  wSpmStatus = phNxpEse_SPM_GetState(&current_spm_state);
  if (wSpmStatus == ESESTATUS_SUCCESS) {
    /* Check current_spm_state and update config/Spm status*/
    if ((current_spm_state & SPM_STATE_JCOP_DWNLD) ||
        (current_spm_state & SPM_STATE_WIRED))
      return ESESTATUS_BUSY;

    status = phNxpEse_setJcopDwnldState(JCP_DWNLD_INIT);
    if (status == ESESTATUS_SUCCESS) {
      while (ese_dwnld_retry < ESE_JCOP_OS_DWNLD_RETRY_CNT) {
        ALOGD_IF(ese_debug_enabled, "ESE_JCOP_OS_DWNLD_RETRY_CNT retry count");
        wSpmStatus = phNxpEse_SPM_GetState(&current_spm_state);
        if (wSpmStatus == ESESTATUS_SUCCESS) {
          if ((current_spm_state & SPM_STATE_JCOP_DWNLD)) {
            status = ESESTATUS_SUCCESS;
            break;
          }
        } else {
          status = ESESTATUS_FAILED;
          break;
        }
        phNxpEse_Sleep(
            200000); /*sleep for 200 ms checking for jcop dwnld status*/
        ese_dwnld_retry++;
      }
    }
  }

  ALOGD_IF(ese_debug_enabled, "phNxpEse_checkJcopDwnldState status %x", status);
  return status;
}

/******************************************************************************
 * Function         phNxpEse_Transceive
 *
 * Description      This function update the len and provided buffer
 *
 * Returns          On Success ESESTATUS_SUCCESS else proper error code
 *
 ******************************************************************************/
ESESTATUS phNxpEse_Transceive(phNxpEse_data* pCmd, phNxpEse_data* pRsp) {
  ESESTATUS status = ESESTATUS_FAILED;

  if ((NULL == pCmd) || (NULL == pRsp)) return ESESTATUS_INVALID_PARAMETER;

  if ((pCmd->len == 0) || pCmd->p_data == NULL) {
    ALOGE(" phNxpEse_Transceive - Invalid Parameter no data\n");
    return ESESTATUS_INVALID_PARAMETER;
  } else if (pCmd->len > MAX_SUPPORTED_DATA_SIZE) {
    ALOGE(" phNxpEse_Transceive - Invalid data size \n");
    return ESESTATUS_INVALID_RECEIVE_LENGTH;
  } else if ((ESE_STATUS_CLOSE == nxpese_ctxt.EseLibStatus)) {
    ALOGE(" %s ESE Not Initialized \n", __FUNCTION__);
    return ESESTATUS_NOT_INITIALISED;
  } else if ((ESE_STATUS_BUSY == nxpese_ctxt.EseLibStatus)) {
    ALOGE(" %s ESE - BUSY \n", __FUNCTION__);
    return ESESTATUS_BUSY;
  } else if ((ESE_STATUS_RECOVERY == nxpese_ctxt.EseLibStatus)) {
    ALOGE(" %s ESE - RECOVERY \n", __FUNCTION__);
    return ESESTATUS_RECOVERY_STARTED;
  } else {
    nxpese_ctxt.EseLibStatus = ESE_STATUS_BUSY;
    status = phNxpEseProto7816_Transceive((phNxpEse_data*)pCmd,
                                          (phNxpEse_data*)pRsp);
    if (ESESTATUS_SUCCESS != status) {
      ALOGE(" %s phNxpEseProto7816_Transceive- Failed \n", __FUNCTION__);
      if (ESESTATUS_TRANSCEIVE_FAILED == status) {
        /*MAX WTX reached*/
        nxpese_ctxt.EseLibStatus = ESE_STATUS_RECOVERY;
      } else {
        /*Timeout/ No response*/
        nxpese_ctxt.EseLibStatus = ESE_STATUS_IDLE;
      }
    } else {
      nxpese_ctxt.EseLibStatus = ESE_STATUS_IDLE;
    }
    nxpese_ctxt.rnack_sent = false;

    ALOGD_IF(ese_debug_enabled, " %s Exit status 0x%x \n", __FUNCTION__,
             status);
    return status;
  }
}
/******************************************************************************
 * Function         phNxpEse_coldReset
 *
 * Description      This function power cycles the ESE
 *                  (cold reset by prop. FW command) interface by
 *                  talking to NFC HAL
 *
 *                  Note:
 *                  After cold reset, phNxpEse_init need to be called to
 *                  reset the host AP T=1 stack parameters
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if the operation is
 *successful else
 *                  ESESTATUS_FAILED(1)
 ******************************************************************************/
ESESTATUS phNxpEse_coldReset(void) {
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  ALOGD_IF(ese_debug_enabled, " %s Enter \n", __FUNCTION__);
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_RECOVERY_RESET);
  } else {
    wSpmStatus = ESESTATUS_FAILED;
    ALOGE(" %s Function not supported \n", __FUNCTION__);
  }
  ALOGD_IF(ese_debug_enabled, " %s Exit status 0x%x \n", __FUNCTION__,
           wSpmStatus);
  return wSpmStatus;
}

/******************************************************************************
 * Function         phNxpEse_reset
 *
 * Description      This function reset the ESE interface and free all
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if the operation is
 *successful else
 *                  ESESTATUS_FAILED(1)
 ******************************************************************************/
ESESTATUS phNxpEse_reset(void) {
  ESESTATUS status = ESESTATUS_FAILED;
  unsigned long maxTimer = 0;
#ifdef SPM_INTEGRATED
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
#endif

  /* TBD : Call the ioctl to reset the ESE */
  ALOGD_IF(ese_debug_enabled, " %s Enter \n", __FUNCTION__);
  /* Do an interface reset, don't wait to see if JCOP went through a full power
   * cycle or not */
  status = phNxpEseProto7816_IntfReset(
      (phNxpEseProto7816SecureTimer_t*)&nxpese_ctxt.secureTimerParams);
  if (status) {
    ALOGE("%s Ese status Failed", __FUNCTION__);
  }

  ALOGD_IF(ese_debug_enabled,
           "%s secureTimer1 0x%x secureTimer2 0x%x secureTimer3 0x%x",
           __FUNCTION__, nxpese_ctxt.secureTimerParams.secureTimer1,
           nxpese_ctxt.secureTimerParams.secureTimer2,
           nxpese_ctxt.secureTimerParams.secureTimer3);
  phNxpEse_GetMaxTimer(&maxTimer);
#ifdef SPM_INTEGRATED
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    status = phNxpEse_SPM_DisablePwrControl(maxTimer);
    if (status != ESESTATUS_SUCCESS) {
      ALOGE("%s phNxpEse_SPM_DisablePwrControl: failed", __FUNCTION__);
    }
  }
  if ((nxpese_ctxt.pwr_scheme == PN67T_POWER_SCHEME) ||
      (nxpese_ctxt.pwr_scheme == PN80T_LEGACY_SCHEME)) {
    wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_POWER_RESET);
    if (wSpmStatus != ESESTATUS_SUCCESS) {
      ALOGE("phNxpEse_SPM_ConfigPwr: reset Failed");
    }
  }
#else
  /* if arg ==2 (hard reset)
   * if arg ==1 (soft reset)
   */
  status = phPalEse_ioctl(phPalEse_e_ResetDevice, nxpese_ctxt.pDevHandle, 2);
  if (status != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_reset Failed");
  }
#endif
  ALOGD_IF(ese_debug_enabled, " %s Exit \n", __FUNCTION__);
  return status;
}

/******************************************************************************
 * Function         phNxpEse_resetJcopUpdate
 *
 * Description      This function reset the ESE interface during JCOP Update
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if the operation is
 *successful else
 *                  ESESTATUS_FAILED(1)
 ******************************************************************************/
ESESTATUS phNxpEse_resetJcopUpdate(void) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  uint8_t retry = 0;
#ifdef SPM_INTEGRATED
  unsigned long int num = 0;
#endif

  /* TBD : Call the ioctl to reset the  */
  ALOGD_IF(ese_debug_enabled, " %s Enter \n", __FUNCTION__);

  /* Reset interface after every reset irrespective of
  whether JCOP did a full power cycle or not. */
  do {
    status = phNxpEseProto7816_Reset();
    if (status != ESESTATUS_SUCCESS) phNxpEse_SPM_ConfigPwr(SPM_RECOVERY_RESET);
  } while (status != ESESTATUS_SUCCESS && retry++ < 1);

  /* Retrieving the IFS-D value configured in the config file and applying to
   * Card */
  if (EseConfig::hasKey(NAME_NXP_ESE_IFSD_VALUE)) {
    unsigned long int ifsd_value = 0;
    ifsd_value = EseConfig::getUnsigned(NAME_NXP_ESE_IFSD_VALUE);
    if ((0xFFFF > ifsd_value) && (ifsd_value > 0)) {
      ALOGD_IF(ese_debug_enabled,
               "phNxpEseProto7816_SetIFS IFS adjustment requested with %ld",
               ifsd_value);
      phNxpEse_setIfs(ifsd_value);
    } else {
      ALOGD_IF(ese_debug_enabled,
               "phNxpEseProto7816_SetIFS IFS adjustment argument invalid");
    }
  }
#ifdef SPM_INTEGRATED
#if (NXP_POWER_SCHEME_SUPPORT == true)
  if (EseConfig::hasKey(NAME_NXP_POWER_SCHEME)) {
    num = EseConfig::getUnsigned(NAME_NXP_POWER_SCHEME);
    if ((num == 1) || (num == 2)) {
      ALOGD_IF(ese_debug_enabled, " %s Call Config Pwr Reset \n", __FUNCTION__);
      status = phNxpEse_SPM_ConfigPwr(SPM_POWER_RESET);
      if (status != ESESTATUS_SUCCESS) {
        ALOGE("phNxpEse_resetJcopUpdate: reset Failed");
        status = ESESTATUS_FAILED;
      }
    } else if (num == 3) {
      ALOGD_IF(ese_debug_enabled, " %s Call eSE Chip Reset \n", __FUNCTION__);
      status = phNxpEse_chipReset();
      if (status != ESESTATUS_SUCCESS) {
        ALOGE("phNxpEse_resetJcopUpdate: chip reset Failed");
        status = ESESTATUS_FAILED;
      }
    } else {
      ALOGD_IF(ese_debug_enabled, " %s Invalid Power scheme \n", __FUNCTION__);
    }
  }
#else
  {
    status = phNxpEse_SPM_ConfigPwr(SPM_POWER_RESET);
    if (status != ESESTATUS_SUCCESS) {
      ALOGE("phNxpEse_SPM_ConfigPwr: reset Failed");
      status = ESESTATUS_FAILED;
    }
  }
#endif
#else
  /* if arg ==2 (hard reset)
   * if arg ==1 (soft reset)
   */
  status = phPalEse_ioctl(phPalEse_e_ResetDevice, nxpese_ctxt.pDevHandle, 2);
  if (status != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_resetJcopUpdate Failed");
  }
#endif

  ALOGD_IF(ese_debug_enabled, " %s Exit \n", __FUNCTION__);
  return status;
}
/******************************************************************************
 * Function         phNxpEse_EndOfApdu
 *
 * Description      This function is used to send S-frame to indicate
 *END_OF_APDU
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if the operation is
 *successful else
 *                  ESESTATUS_FAILED(1)
 *
 ******************************************************************************/
ESESTATUS phNxpEse_EndOfApdu(void) {
  ESESTATUS status = ESESTATUS_SUCCESS;
#if (NXP_ESE_END_OF_SESSION == true)
  status = phNxpEseProto7816_Close(
      (phNxpEseProto7816SecureTimer_t*)&nxpese_ctxt.secureTimerParams);
#endif
  return status;
}

/******************************************************************************
 * Function         phNxpEse_chipReset
 *
 * Description      This function is used to reset the ESE.
 *
 * Returns          Always return ESESTATUS_SUCCESS (0).
 *
 ******************************************************************************/
ESESTATUS phNxpEse_chipReset(void) {
  ESESTATUS status = ESESTATUS_FAILED;
  ESESTATUS bStatus = ESESTATUS_FAILED;
  if (nxpese_ctxt.pwr_scheme == PN80T_EXT_PMU_SCHEME) {
    bStatus = phNxpEseProto7816_Reset();
    if (!bStatus) {
      ALOGE("Inside phNxpEse_chipReset, phNxpEseProto7816_Reset Failed");
    }
    status = phPalEse_ioctl(phPalEse_e_ChipRst, nxpese_ctxt.pDevHandle, 6);
    if (status != ESESTATUS_SUCCESS) {
      ALOGE("phNxpEse_chipReset  Failed");
    }
  } else {
    ALOGD_IF(ese_debug_enabled,
             "phNxpEse_chipReset is not supported in legacy power scheme");
  }
  return status;
}

/******************************************************************************
 * Function         phNxpEse_GetOsMode
 *
 * Description      This function is used to get OS mode(JCOP/OSU)
 *
 * Returns          0x01 : JCOP_MODE
 *                  0x02 : OSU_MODE
 *
 ******************************************************************************/
phNxpEseProto7816_OsType_t phNxpEse_GetOsMode(void) {
  return phNxpEseProto7816_GetOsMode();
}

/******************************************************************************
 * Function         phNxpEse_isColdResetRequired
 *
 * Description      This function determines whether hard reset recovery is
 *                  required or not on protocol recovery failure.
 * Returns          TRUE(required)/FALSE(not required).
 *
 ******************************************************************************/
static __inline bool phNxpEse_isColdResetRequired(phNxpEse_initMode mode,
                                                  ESESTATUS status) {
  return (mode == ESE_MODE_OSU && status != ESESTATUS_SUCCESS);
}

/******************************************************************************
 * Function         phNxpEse_doResetProtection
 *
 * Description      This function enables/disables reset protection
 *
 * Returns          SUCCESS(0)/FAIL(-1).
 *
 ******************************************************************************/
ESESTATUS phNxpEse_doResetProtection(bool flag) {
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  ALOGD_IF(ese_debug_enabled, " %s Enter \n", __FUNCTION__);
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    wSpmStatus = phPalEse_ioctl(phPalEse_e_ResetProtection,
                                nxpese_ctxt.pDevHandle, flag);
  } else {
    wSpmStatus = ESESTATUS_FAILED;
    ALOGE(" %s Function not supported \n", __FUNCTION__);
  }
  ALOGD_IF(ese_debug_enabled, " %s Exit status 0x%x \n", __FUNCTION__,
           wSpmStatus);
  return wSpmStatus;
}

/******************************************************************************
 * Function         phNxpEse_deInit
 *
 * Description      This function de-initializes all the ESE protocol params
 *
 * Returns          Always return ESESTATUS_SUCCESS (0).
 *
 ******************************************************************************/
ESESTATUS phNxpEse_deInit(void) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  unsigned long maxTimer = 0;
  unsigned long num = 0;
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0 &&
      (ESE_STATUS_RECOVERY == nxpese_ctxt.EseLibStatus) &&
      ESE_PROTOCOL_MEDIA_SPI != nxpese_ctxt.initParams.mediaType) {
    return status;
  }
  if (nxpese_ctxt.initParams.initMode == ESE_MODE_OSU) {
    phNxpEse_doResetProtection(false);
  }
  /*TODO : to be removed after JCOP fix*/
  if (EseConfig::hasKey(NAME_NXP_VISO_DPD_ENABLED)) {
    num = EseConfig::getUnsigned(NAME_NXP_VISO_DPD_ENABLED);
  }
  if (num == 0 && nxpese_ctxt.nadInfo.nadRx == EUICC_NAD_RX) {
    // do nothing
  } else {
    status = phNxpEseProto7816_Close(
        (phNxpEseProto7816SecureTimer_t*)&nxpese_ctxt.secureTimerParams);
    if (status == ESESTATUS_SUCCESS) {
      ALOGD_IF(ese_debug_enabled,
               "%s secureTimer1 0x%x secureTimer2 0x%x secureTimer3 0x%x",
               __FUNCTION__, nxpese_ctxt.secureTimerParams.secureTimer1,
               nxpese_ctxt.secureTimerParams.secureTimer2,
               nxpese_ctxt.secureTimerParams.secureTimer3);
      phNxpEse_GetMaxTimer(&maxTimer);
#ifdef SPM_INTEGRATED
      if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
        status = phNxpEse_SPM_DisablePwrControl(maxTimer);
        if (status != ESESTATUS_SUCCESS) {
          ALOGE("%s phNxpEseP61_DisablePwrCntrl: failed", __FUNCTION__);
        }
      } else {
        ALOGD_IF(ese_debug_enabled, "Interface reset for DPD");
        status = phNxpEseProto7816_IntfReset(
            (phNxpEseProto7816SecureTimer_t*)&nxpese_ctxt.secureTimerParams);
        if (status != ESESTATUS_SUCCESS) {
          ALOGE("%s IntfReset Failed ", __FUNCTION__);
        }
      }
#endif
    }
  }
  return status;
}

/******************************************************************************
 * Function         phNxpEse_close
 *
 * Description      This function close the ESE interface and free all
 *                  resources.
 *
 * Returns          Always return ESESTATUS_SUCCESS (0).
 *
 ******************************************************************************/
ESESTATUS phNxpEse_close(ESESTATUS deInitStatus) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  ALOGD_IF(ese_debug_enabled, "phNxpEse_close Enter");
  if ((ESE_STATUS_CLOSE == nxpese_ctxt.EseLibStatus)) {
    ALOGE(" %s ESE Not Initialized \n", __FUNCTION__);
    return ESESTATUS_NOT_INITIALISED;
  }

#ifdef SPM_INTEGRATED
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
#endif

#ifdef SPM_INTEGRATED
  /* Release the Access of  */
  wSpmStatus = phNxpEse_SPM_ConfigPwr(SPM_POWER_DISABLE);
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_ConfigPwr: disabling power Failed");
  } else {
    nxpese_ctxt.spm_power_state = false;
  }

  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    if (ESE_MODE_OSU == nxpese_ctxt.initParams.initMode) {
      status = phNxpEse_setJcopDwnldState(JCP_SPI_DWNLD_COMPLETE);
      if (status != ESESTATUS_SUCCESS) {
        ALOGE("%s: phNxpEse_setJcopDwnldState failed", __FUNCTION__);
      }
    }
  } else {
    if (NULL != nxpese_ctxt.pDevHandle) {
      if (ESE_PROTOCOL_MEDIA_SPI == nxpese_ctxt.initParams.mediaType) {
        ALOGD_IF(ese_debug_enabled, "Inform eSE that trusted Mode is over");
        status = phPalEse_ioctl(phPalEse_e_SetSecureMode,
                                nxpese_ctxt.pDevHandle, 0x00);
        if (status != ESESTATUS_SUCCESS) {
          ALOGE("%s: phPalEse_e_SetSecureMode failed", __FUNCTION__);
        }
        if (ESESTATUS_SUCCESS != phNxpEseProto7816_CloseAllSessions()) {
          ALOGD_IF(ese_debug_enabled, "eSE not responding perform hard reset");
          phNxpEse_SPM_ConfigPwr(SPM_RECOVERY_RESET);
        }
      } else {
        if (nxpese_ctxt.EseLibStatus == ESE_STATUS_RECOVERY ||
            (deInitStatus == ESESTATUS_RESPONSE_TIMEOUT) ||
            ESESTATUS_SUCCESS != phNxpEseProto7816_CloseAllSessions()) {
          ALOGD_IF(ese_debug_enabled, "eSE not responding perform hard reset");
          phNxpEse_SPM_ConfigPwr(SPM_RECOVERY_RESET);
        }
      }
      ALOGD_IF(ese_debug_enabled, "Interface reset for DPD");
      status = phNxpEseProto7816_IntfReset(
          (phNxpEseProto7816SecureTimer_t*)&nxpese_ctxt.secureTimerParams);
      if (status == ESESTATUS_TRANSCEIVE_FAILED || status == ESESTATUS_FAILED) {
        ALOGE("%s IntfReset Failed, perform hard reset", __FUNCTION__);
        // max wtx or no response of interface reset after protocol recovery
        phNxpEse_SPM_ConfigPwr(SPM_RECOVERY_RESET);
      }
    }
  }

  wSpmStatus = phNxpEse_SPM_DeInit();
  if (wSpmStatus != ESESTATUS_SUCCESS) {
    ALOGE("phNxpEse_SPM_DeInit Failed");
  }
#endif
  if (NULL != nxpese_ctxt.pDevHandle) {
    phPalEse_close(nxpese_ctxt.pDevHandle);
    phNxpEse_memset(&nxpese_ctxt, 0x00, sizeof(nxpese_ctxt));
    ALOGD_IF(ese_debug_enabled,
             "phNxpEse_close - ESE Context deinit completed");
  }
  /* Return success always */
  return status;
}

/******************************************************************************
 * Function         phNxpEse_read
 *
 * Description      This function write the data to ESE through physical
 *                  interface (e.g. I2C) using the  driver interface.
 *                  Before sending the data to ESE, phNxpEse_write_ext
 *                  is called to check if there is any extension processing
 *                  is required for the SPI packet being sent out.
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if read successful else
 *                  ESESTATUS_FAILED(1)
 *
 ******************************************************************************/
ESESTATUS phNxpEse_read(uint32_t* data_len, uint8_t** pp_data) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  int ret = -1;

  ALOGD_IF(ese_debug_enabled, "%s Enter ..", __FUNCTION__);

  ret = phNxpEse_readPacket(nxpese_ctxt.pDevHandle, nxpese_ctxt.p_read_buff,
                            MAX_DATA_LEN);
  if (ret < 0) {
    ALOGE("PAL Read status error status = %x", status);
    *data_len = 2;
    *pp_data = nxpese_ctxt.p_read_buff;
    status = ESESTATUS_FAILED;
  } else {
    PH_PAL_ESE_PRINT_PACKET_RX(nxpese_ctxt.p_read_buff, (uint16_t)ret);
    *data_len = ret;
    *pp_data = nxpese_ctxt.p_read_buff;
    status = ESESTATUS_SUCCESS;
  }

  ALOGD_IF(ese_debug_enabled, "%s Exit", __FUNCTION__);
  return status;
}

/******************************************************************************
 * Function         phNxpEse_readPacket
 *
 * Description      This function Reads requested number of bytes from
 *                  pn547 device into given buffer.
 *
 * Returns          nNbBytesToRead- number of successfully read bytes
 *                  -1        - read operation failure
 *
 ******************************************************************************/
static int phNxpEse_readPacket(void* pDevHandle, uint8_t* pBuffer,
                               int nNbBytesToRead) {
  bool flushData = false;
  int ret = -1;
  int sof_counter = 0; /* one read may take 1 ms*/
  int total_count = 0, numBytesToRead = 0, headerIndex = 0;

  ALOGD_IF(ese_debug_enabled, "%s Enter", __FUNCTION__);
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    int max_sof_counter = 0;
    /*Max retry to get SOF in case of chaining*/
    if (poll_sof_chained_delay == 1) {
      /*Wait Max for 1.3 sec before retry/recvoery*/
      /*(max_sof_counter(1300) * 10 us) = 1.3 sec */
      max_sof_counter = ESE_POLL_TIMEOUT * 10;
    }
    /*Max retry to get SOF in case of Non-chaining*/
    else {
      /*wait based on config option */
      /*(nadPollingRetryTime * WAKE_UP_DELAY_SN1xx * NAD_POLLING_SCALER_SN1xx)*/
      max_sof_counter = ((ESE_POLL_TIMEOUT * 1000) /
                         (nxpese_ctxt.nadPollingRetryTime *
                          GET_WAKE_UP_DELAY() * NAD_POLLING_SCALER));
    }
    if (nxpese_ctxt.rnack_sent) {
      phPalEse_sleep(nxpese_ctxt.invalidFrame_Rnack_Delay);
    }
    ALOGD_IF(ese_debug_enabled,
             "read() max_sof_counter: "
             "%X ESE_POLL_TIMEOUT %2X",
             max_sof_counter, ESE_POLL_TIMEOUT);
    do {
      ret = -1;
      ret = phPalEse_read(pDevHandle, pBuffer, 2);
      if (ret < 0) {
        /*Polling for read on spi, hence Debug log*/
        ALOGD_IF(ese_debug_enabled, "_spi_read() [HDR]errno : %x ret : %X",
                 errno, ret);
      } else {
        if ((pBuffer[0] == nxpese_ctxt.nadInfo.nadRx) ||
            (pBuffer[0] == RECEIVE_PACKET_SOF)) {
          /* Read the HEADR of one byte*/
          ALOGD_IF(ese_debug_enabled, "%s Read HDR SOF + PCB", __FUNCTION__);
          numBytesToRead = 1; /*Read only INF LEN*/
          headerIndex = 1;
          break;
        } else if (((pBuffer[0] == 0x00) || (pBuffer[0] == 0xFF)) &&
                   ((pBuffer[1] == nxpese_ctxt.nadInfo.nadRx) ||
                    (pBuffer[1] == RECEIVE_PACKET_SOF))) {
          /* Read the HEADR of Two bytes*/
          ALOGD_IF(ese_debug_enabled, "%s Read HDR only SOF", __FUNCTION__);
          pBuffer[0] = pBuffer[1];
          numBytesToRead = 2; /*Read PCB + INF LEN*/
          headerIndex = 0;
          break;
        } else if (((pBuffer[0] == 0x00) && (pBuffer[1] == 0x00)) ||
                   ((pBuffer[0] == 0xFF) && (pBuffer[1] == 0xFF))) {
          // LOG(ERROR) << StringPrintf("_spi_read() Buf[0]: %X Buf[1]: %X",
          // pBuffer[0], pBuffer[1]);
        } else if (ret >= 0) { /* Corruption happened during the receipt from
                                  Card, go flush out the data */
          ALOGE("_spi_read() Corruption Buf[0]: %X Buf[1]: %X ..len=%d",
                pBuffer[0], pBuffer[1], ret);
          break;
        }
      }
      /*If it is Chained packet wait for 100 usec*/
      if (poll_sof_chained_delay == 1) {
        ALOGD_IF(ese_debug_enabled, "%s Chained Pkt, delay read %dus",
                 __FUNCTION__, GET_WAKE_UP_DELAY() * CHAINED_PKT_SCALER);
        phPalEse_sleep(GET_WAKE_UP_DELAY() * CHAINED_PKT_SCALER);
      } else {
        /*DLOG_IF(INFO, ese_debug_enabled)
         << StringPrintf("%s Normal Pkt, delay read %dus", __FUNCTION__,
         WAKE_UP_DELAY_SN1xx * NAD_POLLING_SCALER_SN1xx);*/
        phPalEse_sleep(nxpese_ctxt.nadPollingRetryTime * GET_WAKE_UP_DELAY() *
                       NAD_POLLING_SCALER);
      }
      sof_counter++;
    } while (sof_counter < max_sof_counter);

    /*SOF Read timeout happened, go for frame retransmission*/
    if (sof_counter == max_sof_counter) {
      ret = -1;
    }
    if (ret < 0) {
      /*In case of IO Error*/
      ret = -2;
      pBuffer[0] = 0x64;
      pBuffer[1] = 0xFF;
    } else if ((pBuffer[0] == nxpese_ctxt.nadInfo.nadRx) ||
               (pBuffer[0] == RECEIVE_PACKET_SOF)) {
      ALOGD_IF(ese_debug_enabled, "%s SOF FOUND", __FUNCTION__);
      /* Read the HEADR of one/Two bytes based on how two bytes read A5 PCB or
       * 00 A5*/
      ret =
          phPalEse_read(pDevHandle, &pBuffer[1 + headerIndex], numBytesToRead);
      if (ret < 0) {
        ALOGE("_spi_read() [HDR]errno : %x ret : %X", errno, ret);
        flushData = true;
      } else {
        if ((pBuffer[1] == CHAINED_PACKET_WITHOUTSEQN) ||
            (pBuffer[1] == CHAINED_PACKET_WITHSEQN)) {
          poll_sof_chained_delay = 1;
          ALOGD_IF(ese_debug_enabled, "poll_sof_chained_delay value is %d ",
                   poll_sof_chained_delay);
        } else {
          poll_sof_chained_delay = 0;
          ALOGD_IF(ese_debug_enabled, "poll_sof_chained_delay value is %d ",
                   poll_sof_chained_delay);
        }
        total_count = 3;
        uint8_t pcb;
        phNxpEseProto7816_PCB_bits_t pcb_bits;
        pcb = pBuffer[PH_PROPTO_7816_PCB_OFFSET];

        phNxpEse_memset(&pcb_bits, 0x00, sizeof(phNxpEseProto7816_PCB_bits_t));
        phNxpEse_memcpy(&pcb_bits, &pcb, sizeof(uint8_t));

        /*For I-Frame Only*/
        if (0 == pcb_bits.msb) {
          if (pBuffer[2] != EXTENDED_FRAME_MARKER) {
            nNbBytesToRead = pBuffer[2];
            headerIndex = 3;
          } else {
            ret = phPalEse_read(pDevHandle, &pBuffer[3], 2);
            if (ret < 0) {
              ALOGE("_spi_read() [HDR]errno : %x ret : %X", errno, ret);
              flushData = true;
            } else {
              nNbBytesToRead = (pBuffer[3] << 8);
              nNbBytesToRead = nNbBytesToRead | pBuffer[4];
              /*If I-Frame received with invalid length respond with RNACK*/
              if ((nNbBytesToRead == 0) || (nNbBytesToRead > MAX_DATA_LEN) ||
                  (nNbBytesToRead > phNxpEseProto7816_GetIfs())) {
                ALOGD_IF(ese_debug_enabled, "I-Frame with invalid len == %d",
                         nNbBytesToRead);
                flushData = true;
              } else {
                ALOGE("_spi_read() [HDR]EXTENDED_FRAME_MARKER, ret=%d", ret);
                total_count += 2;
                headerIndex = 5;
              }
            }
          }
        } else {
          /*For Non-IFrame*/
          nNbBytesToRead = (int)pBuffer[2];
          headerIndex = 3;
        }
        if (!flushData) {
          /* Read the Complete data + one byte CRC*/
          ret = phPalEse_read(pDevHandle, &pBuffer[headerIndex],
                              (nNbBytesToRead + 1));
          if (ret < 0) {
            ALOGE("_spi_read() [HDR]errno : %x ret : %X", errno, ret);
            ret = -1;
          } else {
            ret = (total_count + (nNbBytesToRead + 1));
          }
          nxpese_ctxt.rnack_sent = false;
        }
      }
    } else {
      flushData = true;
    }
    if (flushData) {
      /* Received corrupted frame:
         Flushing out data in the Rx buffer so that Card can switch the mode */
      uint16_t ifsd_size = phNxpEseProto7816_GetIfs();
      uint32_t total_frame_size = 0;
      ALOGE("_spi_read() corrupted, IFSD size=%d flushing it out!!", ifsd_size);
      /* If a non-zero byte is received while polling for NAD byte and the byte
         is not a valid NAD byte (0xA5 or 0xB4): 1)  Read & discard (without
         de-asserting SPI CS line) : a.  Max IFSD size + 5 (remaining four
         prologue + one LRC bytes) bytes from eSE  if max IFS size is greater
         than 254 bytes OR b.  Max IFSD size + 3 (remaining two prologue + one
         LRC bytes) bytes from eSE  if max IFS size is less than 255 bytes.
         2) Send R-NACK to request eSE to re-transmit the frame*/
      if (ifsd_size > IFSC_SIZE_SEND) {
        total_frame_size = ifsd_size + 4;
      } else {
        total_frame_size = ifsd_size + 2;
      }
      nxpese_ctxt.rnack_sent = true;
      phPalEse_sleep(nxpese_ctxt.invalidFrame_Rnack_Delay);
      ret = phPalEse_read(pDevHandle, &pBuffer[2], total_frame_size);
      if (ret < 0) {
        ALOGE("_spi_read() [HDR]errno : %x ret : %X", errno, ret);
      } else { /* LRC fail expected for this frame to send R-NACK*/
        ALOGD_IF(
            ese_debug_enabled,
            "_spi_read() SUCCESS  ret : %X LRC fail excpected for this frame",
            ret);
        PH_PAL_ESE_PRINT_PACKET_RX(pBuffer, ret);
      }
      pBuffer[0] = 0x90;
      pBuffer[1] = RECEIVE_PACKET_SOF;
      ret = 0x02;
      phPalEse_sleep(nxpese_ctxt.invalidFrame_Rnack_Delay);
    }
  } else {
    ret = phNxpEse_readPacket_legacy(pDevHandle, pBuffer, nNbBytesToRead);
  }
  ALOGD_IF(ese_debug_enabled, "%s Exit ret = %d", __FUNCTION__, ret);
  return ret;
}

/******************************************************************************
 * Function         phNxpEse_readPacket_legacy
 *
 * Description      This function Reads requested number of bytes from
 *                  pn547 device into given buffer.
 *
 * Returns          nNbBytesToRead- number of successfully read bytes
 *                  -1        - read operation failure
 *
 ******************************************************************************/
static int phNxpEse_readPacket_legacy(void* pDevHandle, uint8_t* pBuffer,
                                      int nNbBytesToRead) {
  int ret = -1;
  int sof_counter = 0; /* one read may take 1 ms*/
  int total_count = 0, numBytesToRead = 0, headerIndex = 0;
  do {
    sof_counter++;
    ret = -1;
    ret = phPalEse_read(pDevHandle, pBuffer, 2);
    if (ret < 0) {
      /*Polling for read on spi, hence Debug log*/
      ALOGD_IF(ese_debug_enabled, "_spi_read() [HDR]errno : %x ret : %X", errno,
               ret);
    }
    if (pBuffer[0] == RECEIVE_PACKET_SOF) {
      /* Read the HEADR of one byte*/
      ALOGD_IF(ese_debug_enabled, "%s Read HDR", __FUNCTION__);
      numBytesToRead = 1;
      headerIndex = 1;
      break;
    } else if (pBuffer[1] == RECEIVE_PACKET_SOF) {
      /* Read the HEADR of Two bytes*/
      ALOGD_IF(ese_debug_enabled, "%s Read HDR", __FUNCTION__);
      pBuffer[0] = RECEIVE_PACKET_SOF;
      numBytesToRead = 2;
      headerIndex = 0;
      break;
    }
    /*If it is Chained packet wait for 100 usec*/
    if (poll_sof_chained_delay == 1) {
      ALOGD_IF(ese_debug_enabled, "%s Chained Pkt, delay read %dus",
               __FUNCTION__, GET_WAKE_UP_DELAY() * CHAINED_PKT_SCALER);
      phPalEse_sleep(GET_WAKE_UP_DELAY() * CHAINED_PKT_SCALER);
    } else {
      ALOGD_IF(ese_debug_enabled, "%s Normal Pkt, delay read %dus",
               __FUNCTION__, GET_WAKE_UP_DELAY() * NAD_POLLING_SCALER);
      phPalEse_sleep(GET_WAKE_UP_DELAY() * NAD_POLLING_SCALER);
    }
  } while (sof_counter < ESE_NAD_POLLING_MAX);
  if (pBuffer[0] == RECEIVE_PACKET_SOF) {
    ALOGD_IF(ese_debug_enabled, "%s SOF FOUND", __FUNCTION__);
    /* Read the HEADR of one/Two bytes based on how two bytes read A5 PCB or
     * 00 A5*/
    ret = phPalEse_read(pDevHandle, &pBuffer[1 + headerIndex], numBytesToRead);
    if (ret < 0) {
      ALOGE("_spi_read() [HDR]errno : %x ret : %X", errno, ret);
    }
    if ((pBuffer[1] == CHAINED_PACKET_WITHOUTSEQN) ||
        (pBuffer[1] == CHAINED_PACKET_WITHSEQN)) {
      poll_sof_chained_delay = 1;
      ALOGD_IF(ese_debug_enabled, "poll_sof_chained_delay value is %d ",
               poll_sof_chained_delay);
    } else {
      poll_sof_chained_delay = 0;
      ALOGD_IF(ese_debug_enabled, "poll_sof_chained_delay value is %d ",
               poll_sof_chained_delay);
    }
    total_count = 3;
    nNbBytesToRead = (int)pBuffer[2];
    /* Read the Complete data + one byte CRC*/
    ret = phPalEse_read(pDevHandle, &pBuffer[3], (nNbBytesToRead + 1));
    if (ret < 0) {
      ALOGE("_spi_read() [HDR]errno : %x ret : %X", errno, ret);
      ret = -1;
    } else {
      ret = (total_count + (nNbBytesToRead + 1));
    }
  } else if (ret < 0) {
    /*In case of IO Error*/
    ret = -2;
    pBuffer[0] = 0x64;
    pBuffer[1] = 0xFF;
  } else {
    ret = -1;
  }
  return ret;
}

/******************************************************************************
 * Function         phNxpEse_WriteFrame
 *
 * Description      This is the actual function which is being called by
 *                  phNxpEse_write. This function writes the data to ESE.
 *                  It waits till write callback provide the result of write
 *                  process.
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if write successful else
 *                  ESESTATUS_FAILED(1)
 *
 ******************************************************************************/
ESESTATUS phNxpEse_WriteFrame(uint32_t data_len, uint8_t* p_data) {
  ESESTATUS status = ESESTATUS_INVALID_PARAMETER;
  int32_t dwNoBytesWrRd = 0;
  ALOGD_IF(ese_debug_enabled, "Enter %s ", __FUNCTION__);
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    /* TODO where to set the nad id */
    p_data[0] = nxpese_ctxt.nadInfo.nadTx;
  } else {
    p_data[0] = ESE_NAD_TX;
  }
  /* Create local copy of cmd_data */
  phNxpEse_memcpy(nxpese_ctxt.p_cmd_data, p_data, data_len);
  nxpese_ctxt.cmd_len = data_len;

  dwNoBytesWrRd = phPalEse_write(nxpese_ctxt.pDevHandle, nxpese_ctxt.p_cmd_data,
                                 nxpese_ctxt.cmd_len);
  if (-1 == dwNoBytesWrRd) {
    ALOGE(" - Error in SPI Write.....%d\n", errno);
    status = ESESTATUS_FAILED;
  } else {
    status = ESESTATUS_SUCCESS;
    PH_PAL_ESE_PRINT_PACKET_TX(nxpese_ctxt.p_cmd_data, nxpese_ctxt.cmd_len);
  }
  ALOGD_IF(ese_debug_enabled, "Exit %s status %x\n", __FUNCTION__, status);
  return status;
}

/******************************************************************************
 * Function         phNxpEse_getAtr
 *
 * Description      This function retrieves ATR bytes from 7816-3 layer
 *Update.
 *
 * Returns          It returns ESESTATUS_SUCCESS (0) if write successful else
 *                  ESESTATUS_FAILED(1
 *
 ******************************************************************************/
ESESTATUS phNxpEse_getAtr(phNxpEse_data* pATR) {
  ESESTATUS status = ESESTATUS_FAILED;
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    status = phNxpEseProto7816_getAtr(pATR);
  } else {
    ALOGE(" %s - Function not supported\n", __FUNCTION__);
  }
  return status;
}

/******************************************************************************
 * Function         phNxpEse_setIfs
 *
 * Description      This function sets the IFS size to 240/254 support JCOP OS
 *Update.
 *
 * Returns          Always return ESESTATUS_SUCCESS (0).
 *
 ******************************************************************************/
ESESTATUS phNxpEse_setIfs(uint16_t IFS_Size) {
  phNxpEseProto7816_SetIfs(IFS_Size);
  return ESESTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpEse_Sleep
 *
 * Description      This function  suspends execution of the calling thread for
 *           (at least) usec microseconds
 *
 * Returns          Always return ESESTATUS_SUCCESS (0).
 *
 ******************************************************************************/
ESESTATUS phNxpEse_Sleep(uint32_t usec) {
  phPalEse_sleep(usec);
  return ESESTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpEse_memset
 *
 * Description      This function updates destination buffer with val
 *                  data in len size
 *
 * Returns          Always return ESESTATUS_SUCCESS (0).
 *
 ******************************************************************************/
void* phNxpEse_memset(void* buff, int val, size_t len) {
  return phPalEse_memset(buff, val, len);
}

/******************************************************************************
 * Function         phNxpEse_memcpy
 *
 * Description      This function copies source buffer to  destination buffer
 *                  data in len size
 *
 * Returns          Return pointer to allocated memory location.
 *
 ******************************************************************************/
void* phNxpEse_memcpy(void* dest, const void* src, size_t len) {
  return phPalEse_memcpy(dest, src, len);
}

/******************************************************************************
 * Function         phNxpEse_Memalloc
 *
 * Description      This function allocation memory
 *
 * Returns          Return pointer to allocated memory or NULL.
 *
 ******************************************************************************/
void* phNxpEse_memalloc(uint32_t size) {
  return phPalEse_memalloc(size);
  ;
}

/******************************************************************************
 * Function         phNxpEse_calloc
 *
 * Description      This is utility function for runtime heap memory allocation
 *
 * Returns          Return pointer to allocated memory or NULL.
 *
 ******************************************************************************/
void* phNxpEse_calloc(size_t datatype, size_t size) {
  return phPalEse_calloc(datatype, size);
}

/******************************************************************************
 * Function         phNxpEse_free
 *
 * Description      This function de-allocation memory
 *
 * Returns         void.
 *
 ******************************************************************************/
void phNxpEse_free(void* ptr) { return phPalEse_free(ptr); }

/******************************************************************************
 * Function         phNxpEse_GetMaxTimer
 *
 * Description      This function finds out the max. timer value returned from
 *JCOP
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpEse_GetMaxTimer(unsigned long* pMaxTimer) {
  /* Finding the max. of the timer value */
  *pMaxTimer = nxpese_ctxt.secureTimerParams.secureTimer1;
  if (*pMaxTimer < nxpese_ctxt.secureTimerParams.secureTimer2)
    *pMaxTimer = nxpese_ctxt.secureTimerParams.secureTimer2;
  *pMaxTimer = (*pMaxTimer < nxpese_ctxt.secureTimerParams.secureTimer3)
                   ? (nxpese_ctxt.secureTimerParams.secureTimer3)
                   : *pMaxTimer;

  /* Converting timer to millisecond from sec */
  *pMaxTimer = SECOND_TO_MILLISECOND(*pMaxTimer);
  /* Add extra 5% to the timer */
  *pMaxTimer +=
      CONVERT_TO_PERCENTAGE(*pMaxTimer, ADDITIONAL_SECURE_TIME_PERCENTAGE);
  ALOGD_IF(ese_debug_enabled, "%s Max timer value = %lu", __FUNCTION__,
           *pMaxTimer);
  return;
}

/******************************************************************************
 * Function         phNxpEseP61_DisablePwrCntrl
 *
 * Description      This function disables eSE GPIO power off/on control
 *                  when enabled
 *
 * Returns         SUCCESS/FAIL.
 *
 ******************************************************************************/
ESESTATUS phNxpEse_DisablePwrCntrl(void) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  unsigned long maxTimer = 0;
  ALOGE("%s Enter", __FUNCTION__);
  phNxpEse_GetMaxTimer(&maxTimer);
  if (GET_CHIP_OS_VERSION() == OS_VERSION_4_0) {
    status = phNxpEse_SPM_DisablePwrControl(maxTimer);
    if (status != ESESTATUS_SUCCESS) {
      ALOGE("%s phNxpEseP61_DisablePwrCntrl: failed", __FUNCTION__);
    }
  } else {
    ALOGE("%s phNxpEseP61_DisablePwrCntrl: not supported", __FUNCTION__);
    status = ESESTATUS_FAILED;
  }
  return status;
}

/******************************************************************************
 * Function         phNxpEse_getOsVersion
 *
 * Description      This function returns OS version from config file &
 *                  runtime from ATR response
 *
 * Returns         SUCCESS/FAIL.
 *
 ******************************************************************************/
phNxpEse_OsVersion_t phNxpEse_getOsVersion() { return sOsVersion; }

/******************************************************************************
 * Function         phNxpEse_setOsVersion
 *
 * Description      This function sets chip type based on ATR response
 *
 * Returns         None.
 *
 ******************************************************************************/
void phNxpEse_setOsVersion(phNxpEse_OsVersion_t chipType) {
  sOsVersion = chipType;
}

/******************************************************************************
 * Function         phNxpEse_checkFWDwnldStatus
 *
 * Description      This function is  used to  check whether FW download
 *                  is completed or not.
 *
 * Returns          returns  ESESTATUS_SUCCESS or ESESTATUS_BUSY
 *
 ******************************************************************************/
static ESESTATUS phNxpEse_checkFWDwnldStatus(void) {
  ALOGD_IF(ese_debug_enabled, "phNxpEse_checkFWDwnldStatus Enter");
  ESESTATUS wSpmStatus = ESESTATUS_SUCCESS;
  spm_state_t current_spm_state = SPM_STATE_INVALID;
  uint8_t ese_dwnld_retry = 0x00;
  ESESTATUS status = ESESTATUS_FAILED;

  wSpmStatus = phNxpEse_SPM_GetState(&current_spm_state);
  if (wSpmStatus == ESESTATUS_SUCCESS) {
    /* Check current_spm_state and update config/Spm status*/
    while (ese_dwnld_retry < ESE_FW_DWNLD_RETRY_CNT) {
      ALOGD_IF(ese_debug_enabled, "ESE_FW_DWNLD_RETRY_CNT retry count");
      wSpmStatus = phNxpEse_SPM_GetState(&current_spm_state);
      if (wSpmStatus == ESESTATUS_SUCCESS) {
        if ((current_spm_state & SPM_STATE_DWNLD)) {
          status = ESESTATUS_FAILED;
        } else {
          ALOGE("Exit polling no FW Download ..");
          status = ESESTATUS_SUCCESS;
          break;
        }
      } else {
        status = ESESTATUS_FAILED;
        break;
      }
      phNxpEse_Sleep(500000); /*sleep for 500 ms checking for fw dwnld status*/
      ese_dwnld_retry++;
    }
  }

  ALOGD_IF(ese_debug_enabled, "phNxpEse_checkFWDwnldStatus status %x", status);
  return status;
}

/******************************************************************************
 * Function         phNxpEse_GetEseStatus(unsigned char *timer_buffer)
 *
 * Description      This function returns the all three timer
 * Timeout buffer length should be minimum 18 bytes. Response will be in below
 format:
 * <0xF1><Len><Timer Value><0xF2><Len><Timer Value><0xF3><Len><Timer Value>
 *
 * Returns         SUCCESS/FAIL.
 * ESESTATUS_SUCCESS if 0xF1 or 0xF2 tag timeout >= 0 & 0xF3 == 0
 * ESESTATUS_BUSY if 0xF3 tag timeout > 0
 * ESESTATUS_FAILED if any other error

 ******************************************************************************/
ESESTATUS phNxpEse_GetEseStatus(phNxpEse_data* timer_buffer) {
  ESESTATUS status = ESESTATUS_FAILED;

  phNxpEse_SecureTimer_t secureTimerParams;
  uint8_t* temp_timer_buffer = NULL;
  ALOGD_IF(ese_debug_enabled, "%s Enter", __FUNCTION__);
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    ALOGE("%s function not supported", __FUNCTION__);
    return status;
  }
  if (timer_buffer != NULL) {
    timer_buffer->len =
        (sizeof(secureTimerParams.secureTimer1) +
         sizeof(secureTimerParams.secureTimer2) +
         sizeof(secureTimerParams.secureTimer3)) +
        PH_PROPTO_7816_FRAME_LENGTH_OFFSET * PH_PROPTO_7816_FRAME_LENGTH_OFFSET;
    temp_timer_buffer = (uint8_t*)phNxpEse_memalloc(timer_buffer->len);
    timer_buffer->p_data = temp_timer_buffer;

    phNxpEse_memcpy(&secureTimerParams, &nxpese_ctxt.secureTimerParams,
                    sizeof(phNxpEse_SecureTimer_t));

    ALOGD_IF(
        ese_debug_enabled,
        "%s secureTimer1 0x%x secureTimer2 0x%x secureTimer3 0x%x len = %d",
        __FUNCTION__, secureTimerParams.secureTimer1,
        secureTimerParams.secureTimer2, secureTimerParams.secureTimer3,
        timer_buffer->len);

    *temp_timer_buffer++ = PH_PROPTO_7816_SFRAME_TIMER1;
    *temp_timer_buffer++ = sizeof(secureTimerParams.secureTimer1);
    temp_timer_buffer = phNxpEse_GgetTimerTlvBuffer(
        temp_timer_buffer, secureTimerParams.secureTimer1);
    if (temp_timer_buffer != NULL) {
      *temp_timer_buffer++ = PH_PROPTO_7816_SFRAME_TIMER2;
      *temp_timer_buffer++ = sizeof(secureTimerParams.secureTimer2);
      temp_timer_buffer = phNxpEse_GgetTimerTlvBuffer(
          temp_timer_buffer, secureTimerParams.secureTimer2);
      if (temp_timer_buffer != NULL) {
        *temp_timer_buffer++ = PH_PROPTO_7816_SFRAME_TIMER3;
        *temp_timer_buffer++ = sizeof(secureTimerParams.secureTimer3);
        temp_timer_buffer = phNxpEse_GgetTimerTlvBuffer(
            temp_timer_buffer, secureTimerParams.secureTimer3);
        if (temp_timer_buffer != NULL) {
          if (secureTimerParams.secureTimer3 > 0) {
            status = ESESTATUS_BUSY;
          } else {
            status = ESESTATUS_SUCCESS;
          }
        }
      }
    }
  } else {
    ALOGE("%s Invalid timer buffer ", __FUNCTION__);
  }

  ALOGD_IF(ese_debug_enabled, "%s Exit status = 0x%x", __FUNCTION__, status);
  return status;
}

static unsigned char* phNxpEse_GgetTimerTlvBuffer(uint8_t* timer_buffer,
                                                  unsigned int value) {
  short int count = 0, shift = 3;
  unsigned int mask = 0x000000FF;
  if (GET_CHIP_OS_VERSION() != OS_VERSION_4_0) {
    ALOGE("%s function not supported", __FUNCTION__);
  } else {
    ALOGD_IF(ese_debug_enabled, "value = %x \n", value);
    for (count = 0; count < 4; count++) {
      if (timer_buffer != NULL) {
        *timer_buffer = (value >> (shift * 8) & mask);
        ALOGD_IF(ese_debug_enabled, "*timer_buffer=0x%x shift=0x%x",
                 *timer_buffer, shift);
        timer_buffer++;
        shift--;
      } else {
        break;
      }
    }
  }
  return timer_buffer;
}

/******************************************************************************
 * Function         phNxpEse_NotifySEWtxRequest
 *
 * Description      This function notifies SE hal service if it registers
 *                  about WTX ongoing & end status
 *
 * Returns          None
 *
 ******************************************************************************/
void phNxpEse_NotifySEWtxRequest(phNxpEse_wtxState state) {
  if (nxpese_ctxt.fPtr_WtxNtf) {
    (nxpese_ctxt.fPtr_WtxNtf)(state);
  } else {
    ALOGE("%s function not supported", __FUNCTION__);
  }
}

/******************************************************************************
 * Function         phNxpEse_setWtxCountLimit
 *
 * Description      This function sets the counter limit for wtx
 *
 * Returns          None
 *
 ******************************************************************************/
void phNxpEse_setWtxCountLimit(unsigned long int wtxCount) {
  app_wtx_cnt = wtxCount;
}
