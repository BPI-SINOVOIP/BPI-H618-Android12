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

#pragma once

#include <stdbool.h>
#include "bt_types.h"
#include "hcidefs.h"

#define BTU_POST_TO_TASK_NO_GOOD_HORRIBLE_HACK 0x1700 // didn't look used in bt_types...here goes nothing

typedef struct {
  void (*callback)(BT_HDR *);
  uint8_t status;
  BT_HDR *command;
  void *context;
} command_status_hack_t;

typedef struct {
  void (*callback)(BT_HDR *);
  BT_HDR *response;
  void *context;
} command_complete_hack_t;


typedef struct {
  void (*callback)(BT_HDR *);
} post_to_task_hack_t;

void btu_hcif_send_cmd (UNUSED_ATTR UINT8 controller_id, BT_HDR *p_buf);
void btu_hcif_process_event (UNUSED_ATTR UINT8 controller_id, BT_HDR *p_msg);

void hcif_start_up();
void hcif_shut_down();