/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_HWC_COMMON_H
#define ANDROID_HWC_COMMON_H

#undef LOG_TAG
#define LOG_TAG "RanchuHwc"

#include <android-base/logging.h>
#include <inttypes.h>
#include <log/log.h>

// Uncomment to enable additional debug logging.
//#define DEBUG_RANCHU_HWC

#if defined(DEBUG_RANCHU_HWC)
#define DEBUG_LOG ALOGE
#else
#define DEBUG_LOG(...) ((void)0)
#endif

#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

#endif
