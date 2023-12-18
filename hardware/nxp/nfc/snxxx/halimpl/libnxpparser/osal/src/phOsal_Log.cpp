/*
 * Copyright (C) 2017-2019 NXP Semiconductors
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

#include <log/log.h>

#include "phOsal_Posix.h"

/*Global log level to filter the type of logs to be published*/
static phOsal_eLogLevel_t geLogLevel = PHOSAL_LOGLEVEL_NONE;

/**
 * Logging levels
 * This function set the type of logs to be enabled
 *
 * \param[in] eLogLevel  Type of logs to be enabled(info,errors,debug,data)
 *
 */
void phOsal_SetLogLevel(phOsal_eLogLevel_t eLogLevel) {
  geLogLevel = eLogLevel;
  phOsal_LogDebugU32h((const uint8_t*)"Osal>Log Level set to:",
                      (uint32_t)eLogLevel);
}

/**
 * Log Error
 * This function prints the error message specified
 * appropriate log level for error needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 *
 */
void phOsal_LogError(const uint8_t* pbMsg) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_ERROR) ALOGE("%s", pbMsg);
}

/**
 * Log Error
 * This function prints the error message specified along with 32bit value in
 * Hex appropriate log level for error needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogErrorU32h(const uint8_t* pbMsg, uint32_t wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_ERROR) ALOGE("%s:0x%x", pbMsg, wValue);
}

/**
 * Log Error
 * This function prints the error message specified along with 32bit value in
 * decimal appropriate log level for error needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 * \param[in] wValue defines value to be printed
 */
void phOsal_LogErrorU32d(const uint8_t* pbMsg, uint32_t wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_ERROR) ALOGE("%s:%d", pbMsg, wValue);
}

/**
 * Log Error String
 * This function prints the error message specified along with string
 * appropriate log level for error needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 * \param[in] pbString defines pointer to string to be printed
 */
void phOsal_LogErrorString(const uint8_t* pbMsg, const uint8_t* pbString) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_ERROR) ALOGE("%s:%s", pbMsg, pbString);
}

/**
 * Log Information
 * This function prints the information /warning specified
 * appropriate log level for info needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 *
 */
void phOsal_LogInfo(const uint8_t* pbMsg) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO) ALOGI("%s", pbMsg);
}

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in Hex appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogInfoU32h(const uint8_t* pbMsg, uint32_t wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO) ALOGI("%s:0x%x", pbMsg, wValue);
}

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in Hex appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg   defines log message to be printed
 * \param[in] wValue1 defines value to be printed
 * \param[in] wValue2 defines value to be printed
 *
 */
void phOsal_LogInfoU32hh(const uint8_t* pbMsg, uint32_t wValue1,
                         uint32_t wValue2) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO)
    ALOGI("%s:0x%x 0x%x", pbMsg, wValue1, wValue2);
}

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in float appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogInfo32f(const uint8_t* pbMsg, float wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO) ALOGI("%s:%f", pbMsg, wValue);
}

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in decimal appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogInfoU32d(const uint8_t* pbMsg, uint32_t wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO) ALOGI("%s:%d", pbMsg, wValue);
}

/**
 * Log Information
 * This function prints the information / warning specified along with 32bit
 * value in decimal appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg   defines log message to be printed
 * \param[in] wValue1 defines value to be printed
 * \param[in] wValue2 defines value to be printed
 *
 */
void phOsal_LogInfoU32dd(const uint8_t* pbMsg, uint32_t wValue1,
                         uint32_t wValue2) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO)
    ALOGI("%s:%d.%d", pbMsg, wValue1, wValue2);
}

/**
 * Log Information and String
 * This function prints the information specified along with string
 * appropriate log level for info needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] pbString defines pointer to string to be printed
 */
void phOsal_LogInfoString(const uint8_t* pbMsg, const uint8_t* pbString) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_INFO) ALOGI("%s:%s", pbMsg, pbString);
}

/**
 * Log Debug Information
 * This function prints the Debug log message specified
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 *
 */
void phOsal_LogDebug(const uint8_t* pbMsg) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG) ALOGD("%s", pbMsg);
}

/**
 * Log Debug Information
 * This function prints the Debug log message specified along with 32bit value
 * in hex appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogDebugU32h(const uint8_t* pbMsg, uint32_t wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG) ALOGD("%s:0x%X", pbMsg, wValue);
}

/**
 * Log Debug Information
 * This function prints the Debug log message specified along with 32bit value
 * in decimal appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogDebugU32d(const uint8_t* pbMsg, uint32_t wValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG) ALOGD("%s:%d", pbMsg, wValue);
}

/**
 * Log Debug Information
 * This function prints the Debug log message specified along with pointer value
 * in hex appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] pValue defines value to be printed
 *
 */
void phOsal_LogDebugPtrh(const uint8_t* pbMsg, void* pValue) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG) ALOGD("%s:0x%p", pbMsg, pValue);
}

/**
 * Log Debug Information specified along with the string
 * This function prints the Debug log message specified along with 32bit value
 * in decimal appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogDebugString(const uint8_t* pbMsg, const uint8_t* pbString) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG) ALOGD("%s:%s", pbMsg, pbString);
}

/**
 * Log Buffer data
 * This function prints the data in the buffers provided in Hex
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbBuffer              Buffer data  to be logged
 * \param[in] dwSizeOfBuffer        Size of buffer data.
 * \param[in] pbMsg                 defines message to be printed before
 * printing the buffer data.
 *
 */
void phOsal_LogBuffer(const uint8_t* pbBuffer, uint32_t dwSizeOfBuffer,
                      const uint8_t* pbMsg) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DATA_BUFFERS) {
    uint32_t i = 0;
    ALOGD("%s:BufSize=%d:", pbMsg, dwSizeOfBuffer);
    for (i = 0; i < dwSizeOfBuffer; i++) ALOGD("0x%0.2X", pbBuffer[i]);
  }
}

/**
 * Log function entry point
 * This function prints the function name specified prefixed by Module name
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbModuleName defines log message to be printed
 * \param[in] pbFuncName defines value to be printed
 *
 */
void phOsal_LogFunctionEntry(const uint8_t* pbModuleName,
                             const uint8_t* pbFuncName) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG)
    ALOGD("%s>%s:Enter", pbModuleName, pbFuncName);
}

/**
 * Log function exit point
 * This function prints the function name specified prefixed by Module name
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogFunctionExit(const uint8_t* pbModuleName,
                            const uint8_t* pbFuncName) {
  if (geLogLevel >= PHOSAL_LOGLEVEL_DEBUG)
    ALOGD("%s>%s:Exit", pbModuleName, pbFuncName);
}
