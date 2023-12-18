/*
 * Copyright (C) 2014 The Android Open Source Project
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


#ifndef  __burnboot_h
#define  __burnboot_h

#include "Utils.h"

#define BOOT0_NAND_FEX "boot0_nand.fex"
#define BOOT0_EMMC_FEX "boot0_sdcard.fex"
#define UBOOT_FEX "u-boot.fex"
#define TOC0_FEX "toc0.fex"
#define TOC1_FEX "toc1.fex"

typedef int (*DeviceBurn)(BufferExtractCookie *cookie);
typedef int (*DeviceBurnNew)(BufferExtractCookie *cookie, uint32_t slot, bool updateUboot);

#endif
