/*
 * Copyright 2017-2018,2021 NXP
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

#include "phOsal_Adaptation.h"

#include <stdlib.h>
#include <cstdint>
#include <cstring>

/*
#define LOG_FUNCTION_ENTRY \
            phOsal_LogFunctionEntry((const uint8_t*) \
                 "Adapt",(const uint8_t*)__FUNCTION__)
#define LOG_FUNCTION_EXIT \
            phOsal_LogFunctionExit((const uint8_t*) \
                  "Adapt",(const uint8_t*)__FUNCTION__)
*/

#define LOG_FUNCTION_ENTRY
#define LOG_FUNCTION_EXIT

sphOsalAdapt_Context_t gsOsalAdaptContext;

static void* phOsapAdapt_QueMemAllocCB(void*, uint32_t);
static signed int phOsalAdapt_QueMemFreeCB(void*, void*);

ADAPTSTATUS
phOsalAdapt_InitOsal(void* pvAppContext) {
  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;
  memset((uint8_t*)&gsOsalAdaptContext, 0, sizeof(sphOsalAdapt_Context_t));
  psphOsalAdapt_Context_t pContext = &gsOsalAdaptContext;

  phOsal_SetLogLevel(PHOSAL_LOGLEVEL_DATA_BUFFERS);

  LOG_FUNCTION_ENTRY;

  pContext->pvUpLayerContext = pvAppContext;

  phOsal_LogDebug((const uint8_t*)"Adapt>Initializing OSAL...");

  pContext->sOsalConfig.dwCallbackThreadId = 0L;
  pContext->sOsalConfig.pContext = nullptr;
  pContext->sOsalConfig.pLogFile = (uint8_t*)"phOSALLog";

  dwAdaptStatus = phOsal_Init(&pContext->sOsalConfig);
  if (ADAPTSTATUS_SUCCESS != dwAdaptStatus) {
    phOsal_LogError((const uint8_t*)"Adapt>Osal Init Failed !");
    return dwAdaptStatus;
  }

  phOsal_LogDebug((const uint8_t*)"Adapt>Creating Queue...");

  pContext->sQueueCreatePrms.memHdl = nullptr;
  pContext->sQueueCreatePrms.MemAllocCb = phOsapAdapt_QueMemAllocCB;
  pContext->sQueueCreatePrms.MemFreeCb = phOsalAdapt_QueMemFreeCB;
  pContext->sQueueCreatePrms.wQLength = 20;
  pContext->sQueueCreatePrms.eOverwriteMode = PHOSAL_QUEUE_NO_OVERWRITE;

  dwAdaptStatus = phOsal_QueueCreate(&pContext->pvOsalQueueHandle,
                                     &pContext->sQueueCreatePrms);
  if (dwAdaptStatus != ADAPTSTATUS_SUCCESS) {
    phOsal_LogError((const uint8_t*)"Adapt>Queue Creation Failed !");
    return dwAdaptStatus;
  }

  LOG_FUNCTION_EXIT;
  return dwAdaptStatus;
}

ADAPTSTATUS
phOsalAdapt_DeinitOsal() {
  LOG_FUNCTION_ENTRY;
  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;
  psphOsalAdapt_Context_t pContext = &gsOsalAdaptContext;

  dwAdaptStatus = phOsal_ThreadDelete(pContext->pvOsalTaskHandle);
  if (dwAdaptStatus != ADAPTSTATUS_SUCCESS) {
    phOsal_LogDebugString((const uint8_t*)"Adapt>Unable to Delete Thread",
                          (const uint8_t*)__FUNCTION__);
    phOsal_LogErrorU32h((const uint8_t*)"Status = ", dwAdaptStatus);
    return dwAdaptStatus;
  }

  dwAdaptStatus = phOsal_QueueDestroy(pContext->pvOsalQueueHandle);
  if (dwAdaptStatus != ADAPTSTATUS_SUCCESS) {
    phOsal_LogDebug((const uint8_t*)"Adapt>Unable to Delete Queue");
    phOsal_LogErrorU32h((const uint8_t*)"Status = ", dwAdaptStatus);
    return dwAdaptStatus;
  }
  LOG_FUNCTION_EXIT;
  return dwAdaptStatus;
}

ADAPTSTATUS
phOsalAdapt_StartTask(void* threadFunc, void* pParams) {
  LOG_FUNCTION_ENTRY;

  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;

  psphOsalAdapt_Context_t pContext = &gsOsalAdaptContext;

  phOsal_LogDebug((const uint8_t*)"Adapt>Creating Thread...");

  dwAdaptStatus =
      phOsal_ThreadCreate(&pContext->pvOsalTaskHandle,
                          (pphOsal_ThreadFunction_t)threadFunc, pParams);
  if (dwAdaptStatus != ADAPTSTATUS_SUCCESS) {
    phOsal_LogError((const uint8_t*)"Adapt>Thread Creation Failed !");
    phOsal_LogErrorU32h((const uint8_t*)"Status = ", dwAdaptStatus);
    return dwAdaptStatus;
  }

  LOG_FUNCTION_EXIT;
  return dwAdaptStatus;
}

ADAPTSTATUS
phOsalAdapt_StopTask(__attribute__((unused)) void* pvTaskHandle) {
  LOG_FUNCTION_ENTRY;
  sQueueData_t* psQueueData = nullptr;
  psQueueData = (sQueueData_t*)malloc(sizeof(sQueueData_t));
  memset(psQueueData, 0, sizeof(sQueueData_t));
  // send fake packet
  // psQueueData->buffer = 0;
  psQueueData->len = 0;
  return phOsalAdapt_SendData(psQueueData);
  LOG_FUNCTION_EXIT;
}

ADAPTSTATUS
phOsalAdapt_SendData(psQueueData_t psSendData) {
  LOG_FUNCTION_ENTRY;

  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;

  psphOsalAdapt_Context_t pContext = &gsOsalAdaptContext;

  dwAdaptStatus = phOsal_QueuePush(pContext->pvOsalQueueHandle, psSendData, 0);
  if (dwAdaptStatus != ADAPTSTATUS_SUCCESS) {
    phOsal_LogErrorString((const uint8_t*)"Adapt>Unable to Push to Queue",
                          (const uint8_t*)__FUNCTION__);
    phOsal_LogErrorU32h((const uint8_t*)"Status = ", dwAdaptStatus);
    return dwAdaptStatus;
  }

  phOsal_LogDebug((const uint8_t*)"Adapt>Pushed Data");

  LOG_FUNCTION_EXIT;
  return dwAdaptStatus;
}

psQueueData_t phOsalAdapt_GetData() {
  LOG_FUNCTION_ENTRY;

  ADAPTSTATUS dwAdaptStatus = ADAPTSTATUS_FAILED;

  psQueueData_t psGetData;

  psphOsalAdapt_Context_t pContext = &gsOsalAdaptContext;

  phOsal_LogDebug((const uint8_t*)"Adapt>Waiting for Data");

  dwAdaptStatus =
      phOsal_QueuePull(pContext->pvOsalQueueHandle, (void**)&psGetData, 0);
  if (dwAdaptStatus != ADAPTSTATUS_SUCCESS) {
    phOsal_LogErrorString((const uint8_t*)"Adapt>Error QueuePull ",
                          (const uint8_t*)__FUNCTION__);
    phOsal_LogErrorU32h((const uint8_t*)"Status = ", dwAdaptStatus);
    return nullptr;
  }

  phOsal_LogDebug((const uint8_t*)"Adapt>Received Data");

  LOG_FUNCTION_EXIT;
  return psGetData;
}

void* phOsapAdapt_QueMemAllocCB(__attribute__((unused)) void* memHdl,
                                uint32_t size) {
  LOG_FUNCTION_ENTRY;
  void* addr = malloc(size);
  // phOsal_LogDebugU32h((const uint8_t*)"Adapt>Allocating mem Size", size);
  // phOsal_LogDebugU32h((const uint8_t*)"Adapt>Memory Allocation  Handle=",
  // (size_t)memHdl); phOsal_LogDebugU32h((const uint8_t*)"Adapt>Addr=0x",
  // (size_t)addr);
  LOG_FUNCTION_EXIT;
  return addr;
}

signed int phOsalAdapt_QueMemFreeCB(__attribute__((unused)) void* memHdl,
                                    void* ptrToMem) {
  LOG_FUNCTION_ENTRY;
  // phOsal_LogDebugU32h((const uint8_t*)"Adapt>Freeing mem Handle=",
  // (size_t)memHdl); phOsal_LogDebugU32h((const uint8_t*)"Adapt>Freeing mem
  // Addr=", (size_t)ptrToMem);
  free(ptrToMem);
  LOG_FUNCTION_EXIT;
  return 0;
}
