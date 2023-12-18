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

#pragma once

#include <nativebase/nativebase.h>

int bitsPerPixel(int format);
const char* getHalPixelFormatString(int format);
int dumpBuffer(buffer_handle_t buf, const char* path);
int dumpBuffer(int buf, int w, int h, int bpp, const char* path);


#include <string>
#include <hardware/hwcomposer2.h>

static inline std::string toString(const hwc_color_t& c) {
    char buf[32] = {0};
    sprintf(buf, "0x%16x", ((c.a << 24) | (c.r << 16) | (c.g << 8) | (c.b)));
    return std::string(buf);
}

static inline std::string toString(const buffer_handle_t& handle) {
    char buf[32] = {0};
    sprintf(buf, "%16p", handle);
    return std::string(buf);
}

