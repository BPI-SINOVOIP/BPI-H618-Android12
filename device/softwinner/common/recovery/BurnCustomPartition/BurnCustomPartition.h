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


#ifndef  BURN_CUSTOM_PARTITION_H
#define  BURN_CUSTOM_PARTITION_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <android-base/stringprintf.h>


#include "../Utils.h"

typedef int (*DeviceBurnCustom)(BufferExtractCookie *cookie, std::uint32_t targetSlot, std::string partition);

int burnPartition(BufferExtractCookie *cookie, std::uint32_t targetSlot, std::string partition);

#endif
