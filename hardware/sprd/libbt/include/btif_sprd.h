/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
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

#ifndef BT_LIBBT_INCLUDE_BTIF_SPRD_H_
#define BT_LIBBT_INCLUDE_BTIF_SPRD_H_

#define BT_PROFILE_SPRD_ID "sprd"


typedef struct {
  uint16_t opcode;
  uint16_t param_len;
  uint8_t* p_param_buf;
} tSPRD_VSC_CMPL;


typedef void (*vendor_cmd_callback)(tSPRD_VSC_CMPL* p);


/** BT-Test callback structure. */
typedef struct {
    /** set to sizeof(btav_callbacks_t) */
    size_t      size;
    vendor_cmd_callback vendor_cmd_cb;
} btsprd_callbacks_t;



typedef struct {

    /** set to sizeof(btdut_interface_t) */
    size_t          size;
    /**
     * Register the BtSprd callbacks
     */
    bt_status_t (*init)( btsprd_callbacks_t* callbacks );

    bt_status_t (*vendor_cmd_send)(uint16_t opcode, uint8_t len, uint8_t* buf, void *p);

} btsprd_interface_t;

extern const btsprd_interface_t *btif_sprd_get_interface(void);


#endif  // BT_LIBBT_INCLUDE_BTIF_SPRD_H_
