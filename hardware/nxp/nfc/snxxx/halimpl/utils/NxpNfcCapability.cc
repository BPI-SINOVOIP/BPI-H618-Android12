/******************************************************************************
 *
 *  Copyright 2015-2018,2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
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
#define LOG_TAG "NxpHal"
#include "NxpNfcCapability.h"
#include <phNxpLog.h>

capability* capability::instance = NULL;
tNFC_chipType capability::chipType = pn81T;
tNfc_featureList nfcFL;

capability::capability() {}

capability* capability::getInstance() {
  if (NULL == instance) {
    instance = new capability();
  }
  return instance;
}

tNFC_chipType capability::processChipType(uint8_t* msg, uint16_t msg_len) {
  if ((msg != NULL) && (msg_len != 0)) {
    if (msg[0] == 0x60 && msg[1] == 0x00) {
      if (msg[msg_len - 3] == 0x12 && msg[msg_len - 2] == 0x01)
        chipType = pn81T;
      else if (msg[msg_len - 3] == 0x11 && msg[msg_len - 2] == 0x02)
        chipType = pn553;
      else if (msg[msg_len - 3] == 0x01 && msg[msg_len - 2] == 0x10)
        chipType = sn100u;
      else if (msg[msg_len - 3] == 0x01 && msg[msg_len - 2] == 0x01)
        chipType = sn220u;
    } else if (msg[0] == 0x00) {
      if (msg[offsetFwRomCodeVersion] == 0x01 &&
          msg[offsetFwMajorVersion] == 0x01)
        chipType = sn220u;
      if (msg[offsetFwRomCodeVersion] == 0x01 &&
          msg[offsetFwMajorVersion] == 0x10)
        chipType = sn100u;
    } else if (offsetHwVersion < msg_len) {
      ALOGD("%s HwVersion : 0x%02x", __func__, msg[msg_len - 4]);
      switch (msg[msg_len - 4]) {
        case 0x40:  // PN553 A0
        case 0x41:  // PN553 B0
          chipType = pn553;
          break;

        case 0x50:  // PN553 A0 + P73
        case 0x51:  // PN553 B0 + P73
          chipType = pn80T;
          break;

        case 0x98:
          chipType = pn551;
          break;

        case 0xA8:
        case 0x08:
          chipType = pn67T;
          break;

        case 0x28:
        case 0x48:  // NQ210
          chipType = pn548C2;
          break;

        case 0x18:
        case 0x58:  // NQ220
          chipType = pn66T;
          break;
        case 0xA0:
        case 0xA2:
          chipType = sn100u;
          break;
        default:
          chipType = pn80T;
      }
    } else {
      ALOGD("%s Wrong msg_len. Setting Default ChiptType pn80T", __func__);
      chipType = pn81T;
    }
  }
  ALOGD("%s Product : %s", __func__, product[chipType]);
  return chipType;
}
