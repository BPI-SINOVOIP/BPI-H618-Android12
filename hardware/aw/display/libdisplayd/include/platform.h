/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef DISPLYD_PLATFORM_H_
#define DISPLYD_PLATFORM_H_

#include "device/device_controler.h"
#include "hardware/sunxi_display2.h"
#include "IHWCPrivateService.h"

#define PRIMARY_DEVICE      DISP_OUTPUT_TYPE_HDMI
#define EXTERNAL_DEVICE     DISP_OUTPUT_TYPE_TV

#define DEFAULT_HDMI_OUTPUT_MODE DISP_TV_MOD_1080P_60HZ
#define DEFAULT_CVBS_OUTPUT_MODE DISP_TV_MOD_PAL

#define MIN_MARGIN	( 80)
#define MAX_MARGIN	(100)

#define PROPERTY_NAME_LEN_MAX        (64)
#define DISPLAY_ATTR_PROPERTY_PREFIX "persist.display.attr"
#define DISPLAY_ATTR_COUNT           (9)
#define DISPLAY_ATTR_STRING_FORMAT   "t-m-p-d-r-m.lrtb:%d|%d|%d|%d|%d|%d|%d|%d|%d"

/* Output policy */
#define DISPLAY_POLICY_PROPERTY      "persist.display.policy"
#define DISPLAY_POLICY_TYPE_SINGLE   (1)
#define DISPLAY_POLICY_TYPE_DUAL     (2)
#define DISPLAY_POLICY_TYPE_DEFAULT  DISPLAY_POLICY_TYPE_DUAL

struct OutputDeviceInfo;

DeviceControler* createDevice(int displayEngineIndex, int type,
        sunxi::IHWCPrivateService& client);
int platformGetDeviceConnectState(int type);
HardwareCtrl::EnhanceBase *platformGetEnhanceHandle(int id);

#if defined(_board_dolphin_)
#include "platform/platform-sun8iw7p1.h"
#elif defined(_board_petrel_)
#include "platform/platform-sun50iw6p1.h"
#elif defined(_board_cupid_)
#include "platform/platform-cupid.h"
#else
#include "platform/platform-universal.h"
#endif

#endif
