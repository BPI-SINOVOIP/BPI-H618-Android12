/*
 * Copyright 2017-2021 NXP
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

#include "phOsal_Queue.h"
#include "phOsal_LinkList.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstring>

#ifdef WIN32
#include <phNfcTypes.h>
#endif

//#define LOG_FUNCTION_ENTRY phOsal_LogFunctionEntry((const
// uint8_t*)"Osal",(const uint8_t*)__FUNCTION__) #define LOG_FUNCTION_EXIT
// phOsal_LogFunctionExit((const uint8_t*)"Osal",(const uint8_t*)__FUNCTION__)

#define LOG_FUNCTION_ENTRY
#define LOG_FUNCTION_EXIT

typedef struct phOsal_QueueCtxt_tag {
  void* memHdl;
  void* (*MemAllocCb)(void* memHdl, uint32_t Size);
  int32_t (*MemFreeCb)(void* memHdl, void* ptrToMem);
  void* semPush;
  void* semPull;
  void* qMutex;
  uint32_t wQLength;
  void* linkListHdl;
  phOsal_eQueueOverwriteMode_t eOverwriteMode;

} phOsal_QueueCtxt_t;

/**
 * Creates resources for Queue
 */
OSALSTATUS phOsal_QueueCreate(void** pvQueueHandle,
                              phOsal_QueueCreateParams_t* psQueueCreatePrms)

{
  phOsal_QueueCtxt_t* psQCtxt = NULL;
  void* pvMemHdl;
  OSALSTATUS dwStatus = 0;
  phOsal_ListCreateParams_t sListCreatePrms;

  LOG_FUNCTION_ENTRY;

  /*Validity check*/
  if (!psQueueCreatePrms) {
    return OSALSTATUS_INVALID_PARAMS;
  }

  /*Allocate memory to queue context*/
  pvMemHdl = psQueueCreatePrms->memHdl;
  psQCtxt = (phOsal_QueueCtxt_t*)psQueueCreatePrms->MemAllocCb(
      pvMemHdl, sizeof(phOsal_QueueCtxt_t));
  if (!psQCtxt) {
    return OSALSTATUS_FAILED;
  }
  memset(psQCtxt, 0, sizeof(phOsal_QueueCtxt_t));

  /*Copy required variables from inargs to context*/
  psQCtxt->MemAllocCb = psQueueCreatePrms->MemAllocCb;
  psQCtxt->MemFreeCb = psQueueCreatePrms->MemFreeCb;
  psQCtxt->memHdl = psQueueCreatePrms->memHdl;
  psQCtxt->wQLength = psQueueCreatePrms->wQLength;
  psQCtxt->eOverwriteMode = psQueueCreatePrms->eOverwriteMode;

  /*Create semaphore for handling timeouts*/
  /*Semaphore count is same as queue length*/
  // phOsal_LogDebugU32d((const uint8_t*)"Osal>Creating Sem with
  // Value",psQCtxt->wQLength);
  dwStatus = phOsal_SemaphoreCreate(&psQCtxt->semPush, psQCtxt->wQLength, 0);
  if (dwStatus != 0) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to create semaphore\n");
    return OSALSTATUS_FAILED;
  }

  /*Semaphore count is 0 for Pull from queue*/
  dwStatus = phOsal_SemaphoreCreate(&psQCtxt->semPull, 0, 0);
  if (dwStatus != 0) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to create semaphore\n");
    return OSALSTATUS_FAILED;
  }

  /*Create mutex for protecting counters*/
  dwStatus = phOsal_MutexCreate(&psQCtxt->qMutex);
  if (dwStatus != 0) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to create mutex\n");
    return OSALSTATUS_FAILED;
  }

  /*Allocate data structure for maintaining the queue- i.e., a linked list*/
  sListCreatePrms.memHdl = psQCtxt->memHdl;
  sListCreatePrms.MemAllocCb = psQCtxt->MemAllocCb;
  sListCreatePrms.MemFreeCb = psQCtxt->MemFreeCb;
  dwStatus = phOsal_ListCreate(&psQCtxt->linkListHdl, &sListCreatePrms);
  if (dwStatus != 0) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to create LinkedList\n");
    return OSALSTATUS_FAILED;
  }
  *pvQueueHandle = psQCtxt;
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

/**
 * Destroys resources used for Queue
 */
OSALSTATUS phOsal_QueueDestroy(void* pvQueueHandle) {
  phOsal_QueueCtxt_t* psQCtxt = (phOsal_QueueCtxt_t*)pvQueueHandle;
  void* pvMemHdl;
  OSALSTATUS wStatus = OSALSTATUS_SUCCESS;
  LOG_FUNCTION_ENTRY;

  /*Validity check*/
  if (!pvQueueHandle) {
    return OSALSTATUS_INVALID_PARAMS;
  }

  /*get memory handles*/
  pvMemHdl = psQCtxt->memHdl;

  /*Free up resources used by the queue*/
  /*Destroy mutex and semaphores used*/
  if (phOsal_MutexDelete(psQCtxt->qMutex) ||
      phOsal_SemaphoreDelete(psQCtxt->semPush) ||
      phOsal_SemaphoreDelete(psQCtxt->semPull)) {
    return OSALSTATUS_FAILED;
  }

  /*Destroy linked list used by queue*/
  wStatus = phOsal_ListDestroy(psQCtxt->linkListHdl);
  if (OSALSTATUS_SUCCESS != wStatus) {
    return OSALSTATUS_FAILED;
  }

  /*Free up memory to queue context*/
  psQCtxt->MemFreeCb(pvMemHdl, psQCtxt);
  psQCtxt = NULL;

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

/*Push objects from one end of queue*/
OSALSTATUS phOsal_QueuePush(void* pvQueueHandle, void* pvQueueObj,
                            uint32_t uwTimeoutMs) {
  phOsal_QueueCtxt_t* psQCtxt = (phOsal_QueueCtxt_t*)pvQueueHandle;
  OSALSTATUS dwStatus = OSALSTATUS_SUCCESS;

  LOG_FUNCTION_ENTRY;

  /*Validity check*/
  if (!pvQueueHandle || !pvQueueObj) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Params");
    return OSALSTATUS_INVALID_PARAMS;
  }

  /*Semaphore wait to indicate that one of the queue slots is occupied
   This semaphore will start from value= queue_length
   In case of overflow, This wait condition will ensure that one of the object
   is released if configured for infinite timeout*/
  // phOsal_LogDebugU32h((const uint8_t*)"Osal>Waiting for
  // Sem:",(size_t)&psQCtxt->semPush);
  dwStatus = phOsal_SemaphoreWait(psQCtxt->semPush, uwTimeoutMs);
  if (OSALSTATUS_FAILED == dwStatus) {
    phOsal_LogError((const uint8_t*)"Osal>Sem Wait failed");
    return OSALSTATUS_FAILED;
  }

  // phOsal_LogDebugU32h((const uint8_t*)"Osal>Waiting for
  // Mutex",(size_t)&psQCtxt->qMutex);
  /*Either timeout has occurred or an empty slot exists*/
  phOsal_MutexLock(psQCtxt->qMutex);
  // phOsal_LogDebugU32h((const uint8_t*)"Osal>Got Mutex
  // Lock",(size_t)&psQCtxt->qMutex);

  if (OSALSTATUS_SUCCESS == dwStatus) { /*No overflow-Empty slots exists!!*/
    /*Push the object to empty slots and update the counters*/
    dwStatus = phOsal_ListInsertNode(psQCtxt->linkListHdl, PHOSAL_LIST_POS_TAIL,
                                     pvQueueObj);

    { /*Semaphore post to indicate that an object has been added to queue
      This semaphore will starts from 0 .In case of Underflow this sem post
      will signal the consumer of queue object that an object is ready for
         consumption*/

      if (phOsal_SemaphorePost(psQCtxt->semPull)) {
        return OSALSTATUS_FAILED;
      }
    }

  } else if (OSALSTATUS_SEM_TIMEOUT ==
             dwStatus) { /*Check for overflow,If q overflow is detected,
                           take decision based on the queue mode configured */
    switch (psQCtxt->eOverwriteMode) {
      case PHOSAL_QUEUE_NO_OVERWRITE: {
        phOsal_MutexUnlock(psQCtxt->qMutex);
        return OSALSTATUS_Q_OVERFLOW;
        // break;
      }
      case PHOSAL_QUEUE_OVERWRITE_OLDEST: { /*The oldest buffer will be at the
                                               head of the linked list*/
        /*Delete the object at head and add new object to tail*/

        { /*Delete object at head*/
          void* pvData = NULL;
          dwStatus = phOsal_ListRemoveNode(psQCtxt->linkListHdl,
                                           PHOSAL_LIST_POS_HEAD, &pvData);
          phOsal_LogError(
              (const uint8_t*)"Osal>Overwriting Data in Queue at Head");
        }

        dwStatus |= phOsal_ListInsertNode(psQCtxt->linkListHdl,
                                          PHOSAL_LIST_POS_TAIL, pvQueueObj);
        break;
      }
      case PHOSAL_QUEUE_OVERWRITE_NEWEST: { /*The newest buffer will be at the
                                               tail end
                                               Overwrite  the data at the tail*/
        {                                   /*Delete object at tail*/
          void* pvData = NULL;
          dwStatus = phOsal_ListRemoveNode(psQCtxt->linkListHdl,
                                           PHOSAL_LIST_POS_TAIL, &pvData);
          phOsal_LogError(
              (const uint8_t*)"Osal>Overwriting Data in Queue at Tail");
        }

        dwStatus |= phOsal_ListInsertNode(psQCtxt->linkListHdl,
                                          PHOSAL_LIST_POS_TAIL, pvQueueObj);

        break;
      }
      default: {
        phOsal_MutexUnlock(psQCtxt->qMutex);
        return OSALSTATUS_INVALID_PARAMS;
      }

    } /*switch()*/

  } /*else if(OSAL_TIMEOUT== dwStatus)*/
  else {
    return OSALSTATUS_FAILED;
  }

  phOsal_MutexUnlock(psQCtxt->qMutex);
  LOG_FUNCTION_EXIT;
  return dwStatus;
}

/*Pull object at other end of the queue*/
OSALSTATUS phOsal_QueuePull(void* pvQueueHandle, void** pvQueueObj,
                            uint32_t uwTimeoutMs) {
  phOsal_QueueCtxt_t* psQCtxt = (phOsal_QueueCtxt_t*)pvQueueHandle;
  OSALSTATUS dwStatus = OSALSTATUS_SUCCESS;

  LOG_FUNCTION_ENTRY;

  /*Validity check & copy variables*/
  if (!pvQueueHandle || !pvQueueObj) {
    return OSALSTATUS_INVALID_PARAMS;
  }

  /*Check for underflow,If q underflow is detected,
   take decision based on the mode configured */
  dwStatus = phOsal_SemaphoreWait(psQCtxt->semPull, uwTimeoutMs);
  if (OSALSTATUS_FAILED == dwStatus) {
    return OSALSTATUS_FAILED;
  }

  /*Either timeout has occurred or queue is empty*/
  phOsal_MutexLock(psQCtxt->qMutex);

  /*Check for underflow-Check Pushed Not Pulled objects exist in the queue*/
  if (OSALSTATUS_SUCCESS != dwStatus) { /*List empty - UNDERFLOW !!!*/

    *pvQueueObj = NULL;
    phOsal_MutexUnlock(psQCtxt->qMutex);
    phOsal_LogError((const uint8_t*)"Osal> Timeout Occurred :Q Underflow");
    return OSALSTATUS_Q_UNDERFLOW;
  } else { /*No underflow */
    /*Get the object at the tail of the linked list and update the pointers*/
    dwStatus = phOsal_ListRemoveNode(psQCtxt->linkListHdl, PHOSAL_LIST_POS_HEAD,
                                     pvQueueObj);
    if (OSALSTATUS_SUCCESS != dwStatus) {
      phOsal_MutexUnlock(psQCtxt->qMutex);
      return dwStatus;
    }

    /*Semaphore post to indicate that an object has been freed from queue
    will signal the producer of queue object that an object slot is free */
    if (phOsal_SemaphorePost(psQCtxt->semPush)) {
      return OSALSTATUS_FAILED;
    }
  }

  phOsal_MutexUnlock(psQCtxt->qMutex);
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

/**
 * Destroys resources used for Queue
 */
OSALSTATUS phOsal_QueueFlush(void* pvQueueHandle) {
  // phOsal_QueueCtxt_t *psQCtxt=(phOsal_QueueCtxt_t*)pvQueueHandle;
  OSALSTATUS dwOsalStatus = OSALSTATUS_SUCCESS;
  void* pvQueueData;
  LOG_FUNCTION_ENTRY;

  /*Validity check*/
  if (!pvQueueHandle) {
    return OSALSTATUS_INVALID_PARAMS;
  }

  do {
    dwOsalStatus = phOsal_QueuePull(pvQueueHandle, (void**)&pvQueueData, 1);
    if (dwOsalStatus != OSALSTATUS_Q_UNDERFLOW) {
      phOsal_LogError((const uint8_t*)"Osal> Flushed an object from Q");
    }
  } while (dwOsalStatus != OSALSTATUS_Q_UNDERFLOW);
  free(pvQueueData);

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}
