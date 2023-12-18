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

BOOLEAN btsnd_hcic_set_event_filter (UINT8 filt_type, UINT8 filt_cond_type,
                                     UINT8 *filt_cond, UINT8 filt_cond_len);
BOOLEAN btsnd_hcic_enable_test_mode (void);
BOOLEAN btsnd_hcic_set_event_mask(UINT8 local_controller_id, BT_EVENT_MASK event_mask);
BOOLEAN btsnd_hcic_write_scan_enable (UINT8 flag);
BOOLEAN btsnd_hcic_write_cur_iac_lap (UINT8 num_cur_iac, LAP * const iac_lap);
BOOLEAN btsnd_hcic_reset (UINT8 local_controller_id);
BOOLEAN btsnd_hcic_read_bd_addr (void);
void btsnd_hcic_send_cmd (void *buffer, UINT16 opcode, UINT8 len,
                                 UINT8 *p_data, void *p_cmd_cplt_cback);
