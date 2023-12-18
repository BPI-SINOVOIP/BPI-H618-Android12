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

#include "device_controler.h"

class LcdDevice : public DeviceControler {
public:
    LcdDevice(int displayEngineIndex, sunxi::IHWCPrivateService& client);
   ~LcdDevice();

    int performOptimalConfig(int enable);
    int performDefaultConfig(int enable);
    int getSupportModeList(std::vector<int>& out);
    int getSupportPixelFormat(std::vector<int>& out);
    int getSupportDataspace(std::vector<int>& out);
    bool isSupportMode(int mode);

private:
    void dumpDeviceConfig(const char *prefix, struct disp_device_config *config);
    void dumpHook(android::String8& str) {
        str.append("\n");
    }
};
