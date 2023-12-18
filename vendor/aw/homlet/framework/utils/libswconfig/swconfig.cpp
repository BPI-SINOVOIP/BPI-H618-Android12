/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "SW_Config"
#define LOG_NDEBUG 0
#include <stdio.h>
#include <assert.h>

#include "swconfig.h"

extern "C" uint32_t getBoardPlatform()
{
#ifdef SW_CHIP_PLATFORM
    return SW_CHIP_PLATFORM;
#else
    return UNKNOWN_PLATFORM;
#endif
}

extern "C" uint32_t getBusinessPlatform()
{
#ifdef BUSINESS_PLATFORM
    return BUSINESS_PLATFORM;
#else
    return UNKNOWN_PLATFORM;
#endif
}
