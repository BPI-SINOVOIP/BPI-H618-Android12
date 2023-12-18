/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <mutex>
#include <hardware/hwcomposer2.h>

#include "Debug.h"
#include "disputils.h"
#include "DevicePowerManager.h"

namespace sunxi {

class Disp2PowerManager: public DevicePowerManagerBase {
public:
    const int DISPLAY_DEVICE_UNBLANK = 0;
    const int DISPLAY_DEVICE_BLANK   = 1;

    int setRuntimeState(int state)
    {
        const char* control = nullptr;
        std::lock_guard<std::mutex> lock(mLock);

        switch (state) {
        case HWC2_POWER_MODE_ON:
            ::blankCtrl(0 /* primary display id */, DISPLAY_DEVICE_UNBLANK);
            ++mDriverUsage;
            control = "unblank";
            break;
        case HWC2_POWER_MODE_OFF:
            ::blankCtrl(0 /* primary display id */, DISPLAY_DEVICE_BLANK);
            --mDriverUsage;
            control = "blank";
            break;
        }
        DLOGI("disp2 blank control [%s], driver usage [%d]",
                control, mDriverUsage);
        return 0;
    }

    int setRuntimeStateIdle()
    {
        std::lock_guard<std::mutex> lock(mLock);

        if (mDriverUsage == 1) {
            ::blankCtrl(0 /* primary display id */, DISPLAY_DEVICE_BLANK);
            --mDriverUsage;
            DLOGI("disp2 blank control [blank], driver usage [%d]", mDriverUsage);
        } else if (mDriverUsage > 1 || mDriverUsage < 0) {
            DLOGE("It seems that the runtime pm operation is out of sequence!");
        }
        return 0;
    }

private:
    std::mutex mLock;
    int mDriverUsage = 0;
};

std::unique_ptr<DevicePowerManagerBase> createDevicePowerManager()
{
    return std::make_unique<Disp2PowerManager>();
}

} // namespace sunxi
