/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
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

#ifndef BT_LIBBT_SPRD_BUFFER_ALLOCATOR_H_
#define BT_LIBBT_SPRD_BUFFER_ALLOCATOR_H_

#include "allocator.h"

typedef struct _buffer_hdr
{
    struct _buffer_hdr *p_next;   /* next buffer in the queue */
    uint8_t   q_id;                 /* id of the queue */
    uint8_t   status;               /* FREE, UNLINKED or QUEUED */
    uint8_t   Type;
    uint16_t  size;
} BUFFER_HDR_T;

#define BUFFER_HDR_SIZE     (sizeof(BUFFER_HDR_T))                  /* Offset past header */
#define BUF_STATUS_UNLINKED 1

const allocator_t *buffer_allocator_get_interface();
void GKI_freebuf (void *p_buf);
void *GKI_getbuf (uint16_t size);
#endif
