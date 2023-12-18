/*
 * Copyright (C) 2017-2018 NXP Semiconductors
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

#ifndef PHOSAL_ADAPTATION_H_
#define PHOSAL_ADAPTATION_H_

#include "phOsal_Posix.h"
#include "phOsal_Queue.h"

#ifdef __cplusplus
extern "C" { /* Assume C declarations for C++ */
#endif       /* __cplusplus */

#define ADAPTSTATUS_OK 0
#define ADAPTSTATUS_SUCCESS 0
#define ADAPTSTATUS_INVALID_PARAMS 1
#define ADAPTSTATUS_FAILED 2
#define ADAPTSTATUS_SEM_TIMEOUT 3
#define ADAPTSTATUS_Q_OVERFLOW 4
#define ADAPTSTATUS_Q_UNDERFLOW 5
#define ADAPTSTATUS_NOT_SUPPORTED 6
#define ADAPTSTATUS_NOT_INITIALISED 7
#define ADAPTSTATUS_INSUFFICIENT_RESOURCES 8
#define ADAPTSTATUS_ALREADY_INITIALISED 9

typedef unsigned int ADAPTSTATUS; /* Return values */

#define MAX_BUFFER_DATA 260

typedef struct {
  phOsal_Config_t sOsalConfig;
  phOsal_QueueCreateParams_t sQueueCreatePrms;
  void* pvUpLayerContext;
  void* pvOsalTaskHandle;
  void* pvOsalQueueHandle;
  bool bOsalInitialized;
} sphOsalAdapt_Context_t, *psphOsalAdapt_Context_t;

typedef struct {
  unsigned char buffer[MAX_BUFFER_DATA];
  unsigned short len;
} sQueueData_t, *psQueueData_t;

ADAPTSTATUS phOsalAdapt_InitOsal(void* pvAppContext);
ADAPTSTATUS phOsalAdapt_DeinitOsal();
ADAPTSTATUS phOsalAdapt_StartTask(void* pvTaskFuncPtr, void* pParams);
ADAPTSTATUS phOsalAdapt_StopTask(void* pvTaskHandle);
ADAPTSTATUS phOsalAdapt_GetQueHandle(void** ppvQueHandle);
ADAPTSTATUS phOsalAdapt_GetTaskHandle(void** ppvTaskHandle);
ADAPTSTATUS phOsalAdapt_SendData(psQueueData_t psSendData);
psQueueData_t phOsalAdapt_GetData();

#ifdef __cplusplus
} /* Assume C declarations for C++ */
#endif /* __cplusplus */

#endif /* PHOSAL_INTERFACE_H_ */
