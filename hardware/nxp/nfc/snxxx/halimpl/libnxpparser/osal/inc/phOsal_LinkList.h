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

/*!
 *
 * \file  phOsal_LinkList.h
 * \brief OSAL linked list header file.
 *
 * Project:  NFC OSAL LIB
 */

#ifndef PHOSAL_LINKLIST_
#define PHOSAL_LINKLIST_

#include "phOsal_Posix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _PHOSAL_LIST_POSITION {
  PHOSAL_LIST_POS_HEAD,
  PHOSAL_LIST_POS_TAIL,
  PHOSAL_LIST_POS_CUR,
  PHOSAL_LIST_POS_NEXT,
  PHOSAL_LIST_POS_PREV
} PHOSAL_LIST_POSITION_T;

typedef struct phOsal_ListCreateParams {
  void* memHdl;
  void* (*MemAllocCb)(void* memHdl, uint32_t Size);
  int (*MemFreeCb)(void* memHdl, void* ptrToMem);
} phOsal_ListCreateParams_t;

/**
 * \ingroup grp_osal_lib
 * \brief creates linked list
 *
 * This function creates resources for handling linkedlist
 * \param[out] phListHandle       LinkedList Handle
 * \param[in]  psCreateParams     structure contatinng params to create
 * linkedlist \retval #OSALSTATUS_SUCCESS    OSAL LIB Linkedlist created
 * successfully \retval #OSALSTATUS_FAILED     OSAL LIB failed to create
 * linkedlist
 *
 */
OSALSTATUS phOsal_ListCreate(void** phListHandle,
                             phOsal_ListCreateParams_t* psCreateParams);

/**
 * \ingroup grp_osal_lib
 * \brief inserts a new element in linked list
 *
 * This function inserts node to linkedlist
 * \param[in] pvListHandle       LinkedList Handle
 * \param[in]  eListPos          Position to insert linked list
 * \param[in]  pvData            Data to be inserted in the new node
 * \retval #OSALSTATUS_SUCCESS    node inserted in Linkedlist successfully
 * \retval #OSALSTATUS_FAILED     node insertion in  Linkedlist failed
 *
 */
OSALSTATUS phOsal_ListInsertNode(void* pvListHandle,
                                 PHOSAL_LIST_POSITION_T eListPos, void* pvData);

/**
 * \ingroup grp_osal_lib
 * \brief removes the node from linked list and provides data of removed node
 *
 * This function remove node from linkedlist
 * \param[in] pvListHandle       LinkedList Handle
 * \param[in]  eListPos          Position to remove node from linked list
 * \param[in]  ppvData           Pointer to data of removed node
 * \retval #OSALSTATUS_SUCCESS    node removed in Linkedlist successfully
 * \retval #OSALSTATUS_FAILED     node removal in  Linkedlist failed
 *
 */
OSALSTATUS phOsal_ListRemoveNode(void* pvListHandle,
                                 PHOSAL_LIST_POSITION_T eListPos,
                                 void** ppvData);

/**
 * \ingroup grp_osal_lib
 * \brief Flush all objects/nodes in the linked list
 *
 * This function removes all nodes from linkedlist
 * \param[in] pvListHandle       LinkedList Handle
 * \retval #OSALSTATUS_SUCCESS    node removed in Linkedlist successfully
 * \retval #OSALSTATUS_FAILED     node removal in  Linkedlist failed
 *
 */
OSALSTATUS phOsal_ListFlush(void* pvListHandle);

/**
 * \ingroup grp_osal_lib
 * \brief Destroys the linked list
 *
 * This function deletes the linkedlist
 * \param[in] pvListHandle       LinkedList Handle
 * \retval #OSALSTATUS_SUCCESS    node removed in Linkedlist successfully
 * \retval #OSALSTATUS_FAILED     node removal in  Linkedlist failed
 *
 */
OSALSTATUS phOsal_ListDestroy(void* pvListHandle);

#ifdef __cplusplus
} /* End of extern "C" { */
#endif /* __cplusplus */

#endif
