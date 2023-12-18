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

#ifndef  __utils_h
#define  __utils_h

#include <sys/types.h>

#define FLASH_TYPE_NAND  0
#define FLASH_TYPE_SD1  1
#define FLASH_TYPE_SD2  2
#define FLASH_TYPE_UNKNOW	-1

#define STAMP_VALUE             0x5F0A6C39


typedef struct {
    unsigned char* buffer;
    long len;
} BufferExtractCookie;

int check_soc_is_secure(void) ;

int getFlashType();

int getBufferExtractCookieOfFile(const char* path, BufferExtractCookie* cookie);

int checkBoot0Sum(BufferExtractCookie* cookie);
int checkUbootSum(BufferExtractCookie* cookie);
int checkBoot1Sum(BufferExtractCookie* cookie);

int getUbootstartsector(BufferExtractCookie* cookie);

int genBoot0CheckSum(void *cookie);

void clearCache();

#endif
