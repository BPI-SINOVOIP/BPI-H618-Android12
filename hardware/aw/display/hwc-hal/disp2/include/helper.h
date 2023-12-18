/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef _sunxi_helper_h_
#define _sunxi_helper_h_

#include <cutils/native_handle.h>
#include <system/graphics.h>
#include <hardware/graphics-sunxi.h>

#include "hardware/sunxi_display2.h"
#include "ScreenTransform.h"

#if (defined(_board_petrel_)             \
    || defined(_board_apollo_)           \
    || (TARGET_BOARD_PLATFORM == cupid)  \
    || (TARGET_BOARD_PLATFORM == eros)   \
    || (TARGET_BOARD_PLATFORM == mercury))
#define GRALLOC_SUNXI_METADATA_BUF
#endif

disp_pixel_format toDispFormat(int format);
bool compositionEngineV2FormatFilter(int format);
bool isAfbcBuffer(buffer_handle_t buf);
bool isHdrpBuffer(buffer_handle_t buf);
bool isPhysicalContinuousBuffer(buffer_handle_t buf);
bool hardwareScalerCheck(
        int defreq, const sunxi::ScreenTransform& transform,
        bool isyuv, const hwc_rect_t& crop, const hwc_rect_t& frame);
bool afbcBufferScaleCheck(
        int defreq, const sunxi::ScreenTransform& transform,
        const hwc_rect_t& crop, const hwc_rect_t& frame);
void hw_layer_info_revise(disp_layer_info2* info);

#endif
