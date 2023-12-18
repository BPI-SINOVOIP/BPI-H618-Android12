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

#ifndef __DISPLAYD_DEBUG_H__
#define __DISPLAYD_DEBUG_H__

#include <utils/Log.h>
#include <utils/String8.h>

#define TAG_UNDEF   "xxxxxxx: "
#define TAG_HPD     "hotplug: "
#define TAG_DEVICE  " device: "
#define TAG_MANAGER "manager: "
#define TAG_SERVICE "service: "
#define TAG_EDID    "   edid: "

#ifndef _DEBUG_TAG
#define _DEBUG_TAG TAG_UNDEF
#endif

#define dd_info(x, ...)  ALOGW(_DEBUG_TAG x, ##__VA_ARGS__)
#define dd_error(x, ...) ALOGE(_DEBUG_TAG x, ##__VA_ARGS__)

#define DEBUG
#ifdef  DEBUG
#define dd_debug(x, ...) ALOGD(_DEBUG_TAG x, ##__VA_ARGS__)
#else
#define dd_debug(x, ...)
#endif

#endif

