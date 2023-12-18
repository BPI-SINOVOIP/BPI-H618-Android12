/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef __HARDWARE_CTRL_H__
#define __HARDWARE_CTRL_H__

#include "utils.h"
#include "debug.h"

class HardwareCtrl {
public:
    HardwareCtrl(int hardwareId);
   ~HardwareCtrl();

    int getMode();
    int setMode(int mode);
    int checkModeSupport(int mode);
    int getDeviceConfig(struct disp_device_config *config);
    int setDeviceConfig(struct disp_device_config *config);

    class EnhanceBase {
    public:
        enum _enhance_component {
            kEnhanceMode       = 0,
            kEnhanceBright     = 1,
            kEnhanceContrast   = 2,
            kEnhanceDenoise    = 3,
            kEnhanceDetail     = 4,
            kEnhanceEdge       = 5,
            kEnhanceSaturation = 6,
        };

        virtual int getEnhanceComponent(int option) = 0;
        virtual int setEnhanceComponent(int option, int t) = 0;
        virtual int setup() = 0;
        virtual ~EnhanceBase() {}
    };

    EnhanceBase * getEnhanceHandle() {
        return mEnhance;
    }

private:
    int dispioctl(int request, unsigned long arg1,
                  unsigned long arg2 = 0, unsigned long arg3 = 0) const;

    const int mHardwareId;
    EnhanceBase* mEnhance;
    int mDispFd;
};

#endif
