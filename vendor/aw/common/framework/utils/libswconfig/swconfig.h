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

#ifndef __AW_CONFIG__
#define __AW_CONFIG__

#ifdef __cplusplus
extern "C"
{
#endif

#define PLATFORM_CMCCWASU    0x08
#define PLATFORM_ALIYUN        0x09
#define PLATFORM_TVD        0x0A
#define PLATFORM_IPTV        0x0B
#define UNKNOWN_PLATFORM    0xFF

uint32_t getBoardPlatform();
uint32_t getBusinessPlatform();

#ifdef __cplusplus
}
#endif

#endif    // __AW_CONFIG__

