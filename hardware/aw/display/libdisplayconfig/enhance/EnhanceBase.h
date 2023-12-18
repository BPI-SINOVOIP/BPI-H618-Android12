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

#ifndef SUNXI_ENHANCE_BASE_H
#define SUNXI_ENHANCE_BASE_H

#include <memory>

namespace sunxi {

class EnhanceBase {
public:
    enum _enhance_component {
        kEnhanceMode        = 0,
        kEnhanceBright      = 1,
        kEnhanceContrast    = 2,
        kEnhanceDenoise     = 3,
        kEnhanceDetail      = 4,
        kEnhanceEdge        = 5,
        kEnhanceSaturation  = 6,
        kColorTemperature   = 7,
    };

    enum smart_backlight_mode_ {
        kSmartBackLightOff  = 0,
        kSmartBackLightOn   = 1,
        kSmartBackLightDemo = 2,
    };

    // enhance mode define in:
    //     vendor/aw/public/package/display/DisplayService.cpp
    enum enhance_mode_ {
        kEnhanceModeDisable = 0,
        kEnhanceModeEnable  = 1,
        kEnhanceModeDemo    = 2,
    };

    virtual int setup() = 0;
    virtual int getEnhanceComponent(int option) = 0;
    virtual int setEnhanceComponent(int option, int t) = 0;
    virtual int setSmartBackLight(int mode) = 0;
    virtual ~EnhanceBase() = default;
};

std::unique_ptr<EnhanceBase> createEnhanceHandle();

} // namespace sunxi
#endif
