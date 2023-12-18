/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "audio_use_case"
//#define LOG_NDEBUG 0

#include <cutils/list.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>

#include "usecase.h"

use_case_t get_use_case()
{
    use_case_t ret = UC_NODE;

    char val[PROPERTY_VALUE_MAX];

    property_get("ro.vendor.audio.without.earpiece", val, "true");
    if (!strcmp(val, "false"))
        ret |= UC_EAR;

    property_get("ro.vendor.dmic.used", val, "false");
    if (!strcmp(val, "true") || !strcmp(val, "1"))
        ret |= UC_DMIC;

    property_get("ro.vendor.spk_dul.used", val, "false");
    if (!strcmp(val, "true") || !strcmp(val, "1"))
        ret |= UC_DUAL_SPK;

    property_get("ro.vendor.sw.audio.codec_plan_name", val, "PLAN_PAD");
    if (!strcmp(val, "PLAN_TWO") || !strcmp(val, "PLAN_DPHONE"))
        ret |= UC_DPHONE;

    ALOGD("%s: use_case=%#x", __func__, ret);
    return ret;
}
