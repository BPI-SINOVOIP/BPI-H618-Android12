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

#include "debug.h"
#include "utils.h"
#include "platform.h"
#include "device/device_lcd.h"

LcdDevice::LcdDevice(int displayEngineIndex, sunxi::IHWCPrivateService& client)
  : DeviceControler(displayEngineIndex, DISP_OUTPUT_TYPE_LCD, client)
{
    dd_info("Device lcd init");
}

LcdDevice::~LcdDevice()
{

}

int LcdDevice::getSupportModeList(std::vector<int>& out)
{
    out.push_back(0);
    return 0;
}


int LcdDevice::getSupportPixelFormat(std::vector<int>& out)
{
    out.clear();
    return 0;
}

int LcdDevice::getSupportDataspace(std::vector<int>& out)
{
    out.clear();
    return 0;
}

bool LcdDevice::isSupportMode(int mode)
{
    return true;
}

int LcdDevice::performOptimalConfig(int enable)
{
    return performDefaultConfig(enable);
}

int LcdDevice::performDefaultConfig(int enable)
{
    if (enable == 0) {
        mConnectState.setPending(0);
        mConnectState.latch();
        return 0;
    }

    if (platformGetDeviceConnectState(mType) == 0) {
        /* Not connected yet, just return */
        dd_info("LCD performDefaultConfig: not connected yet");
        return 0;
    }

    int dirty = 0;
    struct disp_device_config config;
    struct disp_device_config newconfig;
    getDeviceConfig(&config);

    if (config.type != DISP_OUTPUT_TYPE_LCD) {
        dirty++;
        newconfig = config;
        newconfig.mode   = (enum disp_tv_mode)0;
        newconfig.type   = DISP_OUTPUT_TYPE_LCD;
        newconfig.format = DISP_CSC_TYPE_RGB;
        newconfig.bits   = DISP_DATA_8BITS;
        newconfig.eotf   = DISP_EOTF_GAMMA22;
        newconfig.range  = DISP_COLOR_RANGE_16_235;
        dd_info("Current output type (%d), switch to lcd output", config.type);
    }

    dumpDeviceConfig("Old config", &config);
    if (dirty) {
        setDeviceConfig(&newconfig);
        dumpDeviceConfig("New config", &newconfig);
    }

    mConnectState.setPending(1);
    mConnectState.latch();
    return 0;
}

void LcdDevice::dumpDeviceConfig(const char *prefix, struct disp_device_config *config)
{
    dd_info("%s: type[%d] mode[%d] pixelformat[%d] bits[%d] eotf[%d] colorspace[%d]",
            prefix,
            config->type, config->mode, config->format, config->bits,
            config->eotf, config->cs);
}

