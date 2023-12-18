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

#include "phOsal_Posix.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

/*
****************************** Macro Definitions ******************************
*/
#define LPVOID void*

//#define LOG_FUNCTION_ENTRY phOsal_LogFunctionEntry((const
// uint8_t*)"Osal",(const uint8_t*)__FUNCTION__) #define LOG_FUNCTION_EXIT
// phOsal_LogFunctionExit((const uint8_t*)"Osal",(const uint8_t*)__FUNCTION__)

#define LOG_FUNCTION_ENTRY
#define LOG_FUNCTION_EXIT
/*
*************************** Function Definitions ******************************
*/

OSALSTATUS phOsal_ThreadCreate(void** hThread,
                               pphOsal_ThreadFunction_t pThreadFunction,
                               void* pParam) {
  int32_t status = 0;
  LOG_FUNCTION_ENTRY;
  if ((NULL == hThread) || (NULL == pThreadFunction)) {
    return OSALSTATUS_INVALID_PARAMS;
  }

  /* Check for successful creation of thread */
  status = pthread_create((pthread_t*)hThread, NULL, pThreadFunction, pParam);
  if (0 != status) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to create Thread");
    return OSALSTATUS_FAILED;
  }

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

uint32_t phOsal_ThreadGetTaskId(void) {
  uint32_t dwThreadId = 0;
  LOG_FUNCTION_ENTRY;
  LOG_FUNCTION_EXIT;
  return dwThreadId;
}

OSALSTATUS phOsal_ThreadDelete(void* hThread) {
  void* pRetVal;
  uint32_t status = 0;
  LOG_FUNCTION_ENTRY;
  if (NULL == hThread) {
    return OSALSTATUS_INVALID_PARAMS;
  }
  status = pthread_join((pthread_t)hThread, &pRetVal);
  if (0 != status) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to delete Thread");
    return OSALSTATUS_FAILED;
  }
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

#ifdef ENABLE_ADVANCED_FUNCS
OSALSTATUS phOsal_ThreadSuspend(void* hThread) {
  OSALSTATUS wSuspendStatus = OSALSTATUS_SUCCESS;
  LOG_FUNCTION_ENTRY;
  LOG_FUNCTION_EXIT;
  return wSuspendStatus;
}

OSALSTATUS phOsal_ThreadWakeUp(void* hThread) {
  OSALSTATUS wResumeStatus = OSALSTATUS_SUCCESS;
  LOG_FUNCTION_ENTRY;
  LOG_FUNCTION_EXIT;
  return wResumeStatus;
}
#endif

OSALSTATUS phOsal_ThreadSetPriority(void* hThread, int32_t sdwPriority) {
  uint32_t dwStatus = 0;
  struct sched_param param;
  int32_t policy;
  LOG_FUNCTION_ENTRY;
  if (NULL == hThread) {
    return OSALSTATUS_INVALID_PARAMS;
  }
  dwStatus = pthread_getschedparam((pthread_t)hThread, &policy, &param);
  if (dwStatus != 0) {
    phOsal_LogErrorU32h(
        (const uint8_t*)"Osal>Unable to get thread params.Error=",
        (uint32_t)dwStatus);
    phOsal_LogErrorString((const uint8_t*)"Osal>",
                          (const uint8_t*)__FUNCTION__);
    return OSALSTATUS_FAILED;
  }
  param.sched_priority = sdwPriority;
  dwStatus = pthread_setschedparam((pthread_t)hThread, policy, &param);
  if (dwStatus != 0) {
    phOsal_LogErrorU32h(
        (const uint8_t*)"Osal>Unable to Set thread Priority.Error=",
        (uint32_t)dwStatus);
    phOsal_LogErrorString((const uint8_t*)"Osal>",
                          (const uint8_t*)__FUNCTION__);
    return OSALSTATUS_FAILED;
  }
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

/*static void * phOsal_ThreadProcedure(void *lpParameter)
{
    return lpParameter;
}*/

OSALSTATUS phOsal_SemaphoreCreate(void** hSemaphore, uint8_t bInitialValue,
                                  __attribute__((unused)) uint8_t bMaxValue) {
  int32_t status = 0;
  LOG_FUNCTION_ENTRY;
  // phOsal_LogInfoU32d((const uint8_t*)"Osal>Sem Max
  // Value:",(uint32_t)bMaxValue);

  if (hSemaphore == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Semaphore Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  *hSemaphore = (sem_t*)malloc(sizeof(sem_t));
  if (*hSemaphore == NULL) {
    phOsal_LogError(
        (const uint8_t*)"Osal>Unable to allocate memory for semaphore");
    return OSALSTATUS_FAILED;
  }

  status = sem_init((sem_t*)*hSemaphore, 0, bInitialValue);
  if (status == -1) {
    phOsal_LogErrorU32d(
        (const uint8_t*)"Osal>Unable to allocate memory for semaphore.Status=",
        (uint32_t)status);
    return OSALSTATUS_FAILED;
  }
  // phOsal_LogInfo((const uint8_t*)"Osal> Semaphore Created");
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_SemaphorePost(void* hSemaphore) {
  int32_t checkval;
  LOG_FUNCTION_ENTRY;
  if (hSemaphore == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Semaphore Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (sem_getvalue((sem_t*)hSemaphore, &checkval) == -1) {
    phOsal_LogError((const uint8_t*)"Osal> Semaphore Not available");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (sem_post((sem_t*)hSemaphore) == -1) {
    phOsal_LogError((const uint8_t*)"Osal> error in sem Post");
    return OSALSTATUS_INVALID_PARAMS;
  }

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_SemaphoreWait(void* hSemaphore, uint32_t timeout_ms) {
  int32_t checkval;
  LOG_FUNCTION_ENTRY;
  if (hSemaphore == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Semaphore Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (sem_getvalue((sem_t*)hSemaphore, &checkval) == -1) {
    phOsal_LogError((const uint8_t*)"Osal> Semaphore Not available");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (timeout_ms == 0) {
    if (sem_wait((sem_t*)hSemaphore) == -1) {
      phOsal_LogError(
          (const uint8_t*)"Osal> Error in Semaphore infinite wait !!");
      return OSALSTATUS_INVALID_PARAMS;
    }
  } else {
    struct timespec xtms;
    int32_t status = 0;
    if (clock_gettime(CLOCK_REALTIME, &xtms) == -1) {
      phOsal_LogError(
          (const uint8_t*)"Osal> Error in Getting current CPU time!!");
      return OSALSTATUS_INVALID_PARAMS;
    }

    /*Extract seconds and nanoseconds information from time in milliseconds*/
    xtms.tv_sec += (time_t)timeout_ms / 1000;
    xtms.tv_nsec += ((long)(timeout_ms % 1000)) * (1000000);

    while ((status = sem_timedwait((sem_t*)hSemaphore, &xtms)) == -1 &&
           errno == EINTR) {
      phOsal_LogError(
          (const uint8_t*)"Osal>Error in sem_timedwait restart it!!");
      continue; /* Restart if interrupted by handler */
    }
    /* Check what happened */
    if (status == -1) {
      if (errno == ETIMEDOUT) {
        phOsal_LogError((const uint8_t*)"Osal>sem_timedwait() timed out");
        return OSALSTATUS_SEM_TIMEOUT;
      } else {
        phOsal_LogError((const uint8_t*)"Osal>sem_timedwait");
        return OSALSTATUS_FAILED;
      }
    } else {
      phOsal_LogInfo((const uint8_t*)"Osal>sem_timedwait() succeeded");
    }
  }
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_SemaphoreDelete(void* hSemaphore) {
  int32_t checkval;
  LOG_FUNCTION_ENTRY;
  if (hSemaphore == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Semaphore Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (sem_getvalue((sem_t*)hSemaphore, &checkval) == -1) {
    phOsal_LogError((const uint8_t*)"Osal> Semaphore Not available");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (sem_destroy((sem_t*)hSemaphore) == -1) {
    phOsal_LogError((const uint8_t*)"Osal> Semaphore Destroy Failed");
    return OSALSTATUS_FAILED;
  }

  free(hSemaphore);
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexCreate(void** hMutex) {
  int32_t status = 0;
  LOG_FUNCTION_ENTRY;

  if (hMutex == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Mutex Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  *hMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  if (*hMutex == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Unable to allocate memory for mutex");
    return OSALSTATUS_FAILED;
  }

  status = pthread_mutex_init((pthread_mutex_t*)*hMutex, 0);
  if (status != 0) {
    phOsal_LogErrorU32d((const uint8_t*)"Osal>Error in Mutex Lock",
                        (uint32_t)status);
    return OSALSTATUS_FAILED;
  }
  // phOsal_LogInfo((const uint8_t*)"Osal> Mutex Created");
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexLock(void* hMutex) {
  LOG_FUNCTION_ENTRY;
  if (hMutex == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Mutex Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (pthread_mutex_lock((pthread_mutex_t*)hMutex) == -1) {
    phOsal_LogError((const uint8_t*)"Osal>Error in Mutex Lock");
    return OSALSTATUS_INVALID_PARAMS;
  }

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexUnlock(void* hMutex) {
  LOG_FUNCTION_ENTRY;
  if (hMutex == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Mutex Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (pthread_mutex_unlock((pthread_mutex_t*)hMutex) == -1) {
    phOsal_LogError((const uint8_t*)"Osal>Error in Mutex UnLock");
    return OSALSTATUS_INVALID_PARAMS;
  }

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexDelete(void* hMutex) {
  LOG_FUNCTION_ENTRY;
  if (hMutex == NULL) {
    phOsal_LogError((const uint8_t*)"Osal>Invalid Mutex Handle");
    return OSALSTATUS_INVALID_PARAMS;
  }

  if (pthread_mutex_destroy((pthread_mutex_t*)hMutex) == -1) {
    phOsal_LogError((const uint8_t*)"Osal>Error in Mutex Destroy");
    return OSALSTATUS_INVALID_PARAMS;
  }

  free(hMutex);

  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_Init(pphOsal_Config_t pOsalConfig) {
  // pphOsal_Config_t config = pOsalConfig;
  memset((uint8_t*)&pOsalConfig, 0, sizeof(pphOsal_Config_t));
  LOG_FUNCTION_ENTRY;
  LOG_FUNCTION_EXIT;
  return OSALSTATUS_SUCCESS;
}

void phOsal_Delay(uint32_t dwDelayInMs) {
  usleep(dwDelayInMs * 1000); /**< Converting milliseconds to Microseconds */
}
