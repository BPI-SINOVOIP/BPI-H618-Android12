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

#ifndef PHOSAL_QUEUE_H_
#define PHOSAL_QUEUE_H_

#include "phOsal_Posix.h"

#ifdef __cplusplus
extern "C" { /* Assume C declarations for C++ */
#endif       /* __cplusplus */

typedef enum phOsal_eQueueOverwriteMode {
  PHOSAL_QUEUE_NO_OVERWRITE,
  PHOSAL_QUEUE_OVERWRITE_OLDEST,
  PHOSAL_QUEUE_OVERWRITE_NEWEST
} phOsal_eQueueOverwriteMode_t;

typedef struct phOsal_QueueCreateParams_tag {
  void* memHdl;
  void* (*MemAllocCb)(void* memHdl, uint32_t Size);
  int (*MemFreeCb)(void* memHdl, void* ptrToMem);
  uint32_t wQLength;
  phOsal_eQueueOverwriteMode_t eOverwriteMode;

} phOsal_QueueCreateParams_t;

/**
 * \ingroup grp_osal_lib
 * \brief creates queue
 *
 * This function creates resources for handling queue
 * \param[out] pvQueueHandle       Queue Handle to be filled
 * \param[in]  psQueueCreatePrms  structure contatinng params to create
 * linkedlist \retval #OSALSTATUS_SUCCESS    OSAL LIB Queue created successfully
 * \retval #OSALSTATUS_FAILED     OSAL LIB failed to create queue
 *
 */
extern OSALSTATUS phOsal_QueueCreate(
    void** pvQueueHandle, phOsal_QueueCreateParams_t* psQueueCreatePrms);
/**
 * \ingroup grp_osal_lib
 * \brief Destroys queue
 *
 * This function destroys resources used for handling queue
 * \param[out] pvQueueHandle       Queue Handle to be filled
 * \retval #OSALSTATUS_SUCCESS    OSAL LIB Queue created successfully
 * \retval #OSALSTATUS_FAILED     OSAL LIB failed to create queue
 *
 */
extern OSALSTATUS phOsal_QueueDestroy(void* pvQueueHandle);

/**
 * \ingroup grp_osal_lib
 * \brief Inserts object into queue
 *
 * This function inserts objects into queue

 * \param[in] pvQueueHandle       Queue Handle to be filled
 * \param[out] pvQueueObj         Queue object ot be inserted
 * \retval #OSALSTATUS_SUCCESS    OSAL LIB Queue created successfully
 * \retval #OSALSTATUS_FAILED     OSAL LIB failed to create queue
 *
 */
extern OSALSTATUS phOsal_QueuePush(void* pvQueueHandle, void* pvQueueObj,
                                   uint32_t u4_time_out_ms);

/**
 * \ingroup grp_osal_lib
 * \brief retrieve objects from queue
 *
 * This function retrieves objects from queue
 * \param[in] pvQueueHandle       Queue Handle to be filled
 * \param[out] ppvQueueObj        Queue object pulled
 * \param[in] u4_time_out_ms      time to wait until an object is found in queue
 * \retval #OSALSTATUS_SUCCESS    OSAL LIB Queue Pulled successfully
 * \retval #OSALSTATUS_FAILED     OSAL LIB failed to Pull object from queue
 * \retval #OSALSTATUS_Q_UNDERFLOW  No objects in Q even after timeout
 */
extern OSALSTATUS phOsal_QueuePull(void* pvQueueHandle, void** ppvQueueObj,
                                   uint32_t u4_time_out_ms);

/**
 * \ingroup grp_osal_lib
 * \brief retrieve objects from queue
 *
 * This function retrieves objects from queue
 * \param[in] pvQueueHandle       Queue Handle to be filled
 * \retval #OSALSTATUS_SUCCESS    OSAL LIB Queue created successfully
 * \retval #OSALSTATUS_FAILED     OSAL LIB failed to create queue
 *
 */
extern OSALSTATUS phOsal_QueueFlush(void* pvQueueHandle);

#ifdef __cplusplus
} /* Assume C declarations for C++ */
#endif /* __cplusplus */

#endif /* __PH_OSAL_QUEUE_H__ */
