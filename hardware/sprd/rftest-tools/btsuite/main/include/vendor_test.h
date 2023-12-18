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
#include "vendor_suite.h"

int bt_test_mode_enable(void);
void bt_test_mode_disable(void);
void BTM_SendHciCommand(UINT16 opcode, UINT8 param_len,
                                      UINT8 *p_param_buf, hci_cmd_complete_cb *p_cb);
