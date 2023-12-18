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

/*
************************* Header Files ****************************************
*/

/* System include files */

#include "phOsal_LinkList.h"

#include <cstring>

/**
 * Structure for link list node
 */
typedef struct phOsal_ListNode {
  struct phOsal_ListNode* ps_next;
  void* pvData;
} phOsal_ListNode_t;

/**
 * Context structure for Link List
 */
typedef struct phOsal_ListCtxt {
  phOsal_ListCreateParams_t sListCreateParams;
  phOsal_ListNode_t* psHead;
  phOsal_ListNode_t* psTail;
  uint32_t dwNumNodes;
} phOsal_ListCtxt_t;

#undef ALOGD
#define ALOGD

static OSALSTATUS phOsali_ListDeleteAllNodes(void* pvListHandle);

/**
 * Create resources for linked list data structure
 */
OSALSTATUS phOsal_ListCreate(void** phListHandle,
                             phOsal_ListCreateParams_t* psCreateParams) {
  phOsal_ListCtxt_t* psListCtxt = NULL;
  void* memHdl;

  /*Input validity check*/
  if (NULL == psCreateParams || NULL == phListHandle) {
    return OSALSTATUS_INVALID_PARAMS;
  } /*    if(NULL == psCreateParams || NULL == ph_ll_handle)*/

  /*Allocate memory to linked list context*/
  memHdl = psCreateParams->memHdl;
  psListCtxt = (phOsal_ListCtxt_t*)psCreateParams->MemAllocCb(
      memHdl, sizeof(phOsal_ListCtxt_t));

  if (NULL == psListCtxt) {
    return OSALSTATUS_FAILED;
  } /*if(NULL == psListCtxt)*/

  memset(psListCtxt, 0, sizeof(phOsal_ListCtxt_t));

  /*Copy required variables from inargs to context*/
  memcpy(&(psListCtxt->sListCreateParams), psCreateParams,
         sizeof(phOsal_ListCreateParams_t));

  /*Initialize context variables*/
  psListCtxt->psHead = NULL;
  psListCtxt->psTail = NULL;
  psListCtxt->dwNumNodes = 0;

  /*Update the link list handle*/
  *phListHandle = psListCtxt;

  return OSALSTATUS_SUCCESS;
}

/**
 * Insert an object/node into linkedlist
 */
OSALSTATUS phOsal_ListInsertNode(void* pvListHandle,
                                 PHOSAL_LIST_POSITION_T eListPos,
                                 void* pvData) {
  phOsal_ListCtxt_t* psListCtxt = (phOsal_ListCtxt_t*)pvListHandle;
  phOsal_ListNode_t* psNodeTba = NULL; /*Node to be added*/

  /*Validity check*/
  if (NULL == pvListHandle) {
    return OSALSTATUS_INVALID_PARAMS;
  } /*if(NULL == pvListHandle)*/

  /*Create new obj node to be added and update its parameters*/
  psNodeTba = (phOsal_ListNode_t*)psListCtxt->sListCreateParams.MemAllocCb(
      psListCtxt->sListCreateParams.memHdl, sizeof(phOsal_ListNode_t));

  psNodeTba->ps_next = NULL;
  psNodeTba->pvData = pvData;

  switch (eListPos) /*Update head and tail pointers based on the position of
                    obj to be added*/
  {
    case PHOSAL_LIST_POS_TAIL: {
      if (NULL == psListCtxt->psTail) {
        /** First node is being added */
        psListCtxt->psHead = psNodeTba;
        psListCtxt->psTail = psNodeTba;
      } else {
        /** Node is being added to the tail of the list */
        psListCtxt->psTail->ps_next = psNodeTba;
        psListCtxt->psTail = psNodeTba;
      } /*if (NULL == psListCtxt->psTail)*/

      break;
    } /*case PHOSAL_LIST_POS_TAIL:*/
    case PHOSAL_LIST_POS_HEAD: {
      if (NULL == psListCtxt->psHead) {
        /** First node is being added */
        psListCtxt->psHead = psNodeTba;
        psListCtxt->psTail = psNodeTba;
      } else {
        /** Node is being added to the head of the list */
        psNodeTba->ps_next = psListCtxt->psHead;
        psListCtxt->psHead = psNodeTba;
      } /*if (NULL == psListCtxt->psHead)*/
      break;
    } /*case PHOSAL_LIST_POS_HEAD:*/
    case PHOSAL_LIST_POS_CUR: {
      return OSALSTATUS_NOT_SUPPORTED;
      ; /*TBD: Currently not supported*/
        // break;
    }   /*case PHOSAL_LIST_POS_CUR:*/
    case PHOSAL_LIST_POS_NEXT: {
      return OSALSTATUS_NOT_SUPPORTED;
      ; /*TBD: Currently not supported*/
        // break;
    }   /*case PHOSAL_LIST_POS_NEXT:*/
    case PHOSAL_LIST_POS_PREV: {
      return OSALSTATUS_NOT_SUPPORTED;
      ; /*TBD: Currently not supported*/
      break;
    } /*case PHOSAL_LIST_POS_PREV:*/
    default: {
      return OSALSTATUS_INVALID_PARAMS;
    } /*default*/
  }   /*switch(eListPos)*/

  psListCtxt->dwNumNodes++;
  return OSALSTATUS_SUCCESS;
}

/**
 * Removes an object/node from linkedlist
 */
OSALSTATUS phOsal_ListRemoveNode(void* pvListHandle,
                                 PHOSAL_LIST_POSITION_T eListPos,
                                 void** ppvData) {
  phOsal_ListCtxt_t* psListCtxt = (phOsal_ListCtxt_t*)pvListHandle;
  phOsal_ListNode_t* psNodeTbd = NULL; /*Node to be deleted*/

  /*Validity check*/
  if (NULL == pvListHandle || NULL == ppvData) {
    return OSALSTATUS_INVALID_PARAMS;
  } /*if(NULL == pvListHandle || NULL == pvData)*/

  switch (eListPos) /*Update head and tail pointers based on the position of
                    obj to be deleted*/
  {
    case PHOSAL_LIST_POS_HEAD: {
      if (NULL == psListCtxt->psHead) { /** No elements in the list*/
        *ppvData = NULL;
        return OSALSTATUS_INVALID_PARAMS;
      } else if (NULL ==
                 psListCtxt->psHead
                     ->ps_next) { /** There is only one object in the list*/
        *ppvData = psListCtxt->psHead->pvData;
        psNodeTbd = psListCtxt->psHead;
        psListCtxt->psHead = NULL;
        psListCtxt->psTail = NULL;
      } else { /*There are more than one element in the list*/
        *ppvData = psListCtxt->psHead->pvData;
        psNodeTbd = psListCtxt->psHead;
        psListCtxt->psHead = psListCtxt->psHead->ps_next;
      } /*if (NULL == psListCtxt->psHead)*/

      break;
    }
    case PHOSAL_LIST_POS_TAIL: {
      if (NULL == psListCtxt->psTail) { /** No elements in the list*/
        *ppvData = NULL;
        return OSALSTATUS_INVALID_PARAMS;
      } else if (NULL ==
                 psListCtxt->psHead
                     ->ps_next) { /** There is only one object in the list*/

        *ppvData = psListCtxt->psHead->pvData;
        psNodeTbd = psListCtxt->psHead;
        psListCtxt->psHead = NULL;
        psListCtxt->psTail = NULL;
      } else { /*There are more than one element in the list*/
        phOsal_ListNode_t* ps_ll_node = NULL;
        phOsal_ListNode_t* ps_ll_node_prev = NULL;

        ps_ll_node = psListCtxt->psHead;

        /*Traverse till the end of the link list*/
        while (NULL != ps_ll_node->ps_next) {
          ps_ll_node_prev = ps_ll_node;
          ps_ll_node = ps_ll_node->ps_next;
        } /*while(NULL != ps_ll_node->ps_next)*/

        *ppvData = ps_ll_node->pvData;
        psNodeTbd = ps_ll_node;
        psListCtxt->psTail = ps_ll_node_prev;

      } /*if (NULL == psListCtxt->psTail)*/
      break;
    }
    case PHOSAL_LIST_POS_CUR: {
      return OSALSTATUS_NOT_SUPPORTED;
      ; /*TBD: Currently not supported*/
        // break;
    }   /*case PHOSAL_LIST_POS_CUR:*/
    case PHOSAL_LIST_POS_NEXT: {
      return OSALSTATUS_NOT_SUPPORTED;
      ; /*TBD: Currently not supported*/
        // break;
    }   /*case PHOSAL_LIST_POS_NEXT:*/
    case PHOSAL_LIST_POS_PREV: {
      return OSALSTATUS_NOT_SUPPORTED;
      ; /*TBD: Currently not supported*/
        // break;
    }   /*case PHOSAL_LIST_POS_PREV:*/
    default: {
      return OSALSTATUS_INVALID_PARAMS;
    } /*default:*/
  }   /*switch(eListPos)*/

  { /*Free up the memory for the node to be deleted*/

    psListCtxt->sListCreateParams.MemFreeCb(
        psListCtxt->sListCreateParams.memHdl, psNodeTbd);

    psListCtxt->dwNumNodes--;
  }
  return OSALSTATUS_SUCCESS;
}

/**
 * Destroy the  linkedlist
 */
OSALSTATUS phOsal_ListDestroy(void* pvListHandle) {
  phOsal_ListCtxt_t* psListCtxt = (phOsal_ListCtxt_t*)pvListHandle;

  /*Validity check*/
  if (NULL == pvListHandle) {
    return OSALSTATUS_INVALID_PARAMS;
  } /*if(NULL == pvListHandle)*/

  /*Delete all nodes*/
  phOsali_ListDeleteAllNodes(psListCtxt);

  { /*Free up memory to queue context*/

    psListCtxt->sListCreateParams.MemFreeCb(
        psListCtxt->sListCreateParams.memHdl, psListCtxt);

    psListCtxt = NULL;
  }

  return OSALSTATUS_SUCCESS;
}

/**
 * Flush all objects/nodes in the linked list
 */
OSALSTATUS phOsal_ListFlush(void* pvListHandle) {
  phOsal_ListCtxt_t* psListCtxt = (phOsal_ListCtxt_t*)pvListHandle;
  /*Validity check*/
  if (NULL == pvListHandle) {
    return OSALSTATUS_INVALID_PARAMS;
  } /*if(NULL == pvListHandle)*/

  /*Delete all nodes*/
  phOsali_ListDeleteAllNodes(psListCtxt);

  /*Update the link list context*/
  psListCtxt->psHead = NULL;
  psListCtxt->psTail = NULL;
  psListCtxt->dwNumNodes = 0;

  return OSALSTATUS_SUCCESS;
}

/**
 * Flush all objects/nodes in the linked list
 */
OSALSTATUS phOsali_ListDeleteAllNodes(void* pvListHandle) {
  phOsal_ListCtxt_t* psListCtxt = (phOsal_ListCtxt_t*)pvListHandle;
  phOsal_ListNode_t* psNodeTbd = NULL; /*Node to be deleted*/

  /*Delete all nodes*/
  if (psListCtxt->psHead) { /*Atleast one element exists in the list*/
    while (NULL !=
           psListCtxt->psHead) { /*Delete all nodes until head becomes null*/
      psNodeTbd = psListCtxt->psHead;

      psListCtxt->psHead = psListCtxt->psHead->ps_next;

      /*Free up memory of the node to be deleted*/
      psListCtxt->sListCreateParams.MemFreeCb(
          psListCtxt->sListCreateParams.memHdl, psNodeTbd);
    } /*while(NULL != psListCtxt->psHead)*/
  }   /*if(psListCtxt->psHead)*/

  return OSALSTATUS_SUCCESS;
}

/* nothing past this point */
