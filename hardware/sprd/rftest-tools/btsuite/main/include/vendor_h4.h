/******************************************************************************
 *
 *  Copyright (C) 2017 Spreadtrum Corporation
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

#ifndef BT_LIBBT_SPRD_VENDOR_H4_H_
#define BT_LIBBT_SPRD_VENDOR_H4_H_

#include "vendor_utils.h"
#include "osi/include/thread.h"


int h4_start_up(const hci_h4_callbacks_t *upper_callbacks, thread_t *upper_thread);
void h4_shut_down(void);
size_t read_data(serial_data_type_t type, uint8_t *buffer, size_t max_size, bool block);
void packet_finished(serial_data_type_t type);
void lmp_assert(void);
void lmp_deassert(void);
uint16_t transmit_data(serial_data_type_t type, uint8_t *data, uint16_t length);
#endif  // BT_LIBBT_SPRD_VENDOR_H4_H_
