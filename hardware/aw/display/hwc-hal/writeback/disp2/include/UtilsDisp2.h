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

#ifndef SUNXI_HWC_UTILS_DISP2_H_
#define SUNXI_HWC_UTILS_DISP2_H_

#include "hardware/sunxi_display2.h"

namespace sunxi {

    class UtilsDisp2 {
    public:
        UtilsDisp2();
        ~UtilsDisp2();
        int startWriteBack(int hwid);
        int stopWriteBack(int hwid);
        int switchToDefault(int hwid, int type, int mode);
        int setDeviceConfigToDefault(int hwid, int type, int mode);
        int getDeviceConfig(int hwid, struct disp_device_config* config);
        int captureCommit2(int hwid, struct disp_capture_info2* info);
        int rtwbCommit(int hwid, unsigned int syncnum, disp_layer_config2* conf,
                int lyrNum, struct disp_capture_info2* info);

    private:
        int mFd;
    };

} // namespace sunxi

#endif // SUNXI_HWC_UTILS_DISP2_H_

