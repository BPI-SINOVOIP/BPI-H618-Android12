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

#ifndef SUNXI_HWC_DISPLAY_MODE_H_
#define SUNXI_HWC_DISPLAY_MODE_H_

#include <unistd.h>

struct ModeInfo {
    uint32_t Mode;       /* sunxi mode, sync from kernel header sunxi_display2.h */
    const char *Name;

    int32_t Width;
    int32_t Height;
    int32_t RefreshRate; /* in Hz */
};

const ModeInfo SunxiModeArray[] = {
    {10, "1920x1080-60Hz", 1920, 1080, 60},
};

#endif
