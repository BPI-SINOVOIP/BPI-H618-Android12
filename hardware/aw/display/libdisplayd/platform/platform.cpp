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

#include <string.h>
#include <cutils/properties.h>
#include <sys/stat.h>
#include "debug.h"
#include "platform.h"
#include "device/device_hdmi.h"
#include "device/device_cvbs.h"
#include "device/device_lcd.h"
#include "output_policy.h"
#include "utils.h"
DeviceControler* createDevice(int displayEnginePortId, int type,
        sunxi::IHWCPrivateService& client)
{
    dd_info("createDevice: type=%d on port %d", type, displayEnginePortId);
    switch (type) {
        case DISP_OUTPUT_TYPE_HDMI:
            return new HdmiDevice(displayEnginePortId, client);
        case DISP_OUTPUT_TYPE_TV:
            return new CvbsDevice(displayEnginePortId, client);
        case DISP_OUTPUT_TYPE_LCD:
            return new LcdDevice(displayEnginePortId, client);
        default:
            break;
    }
    return nullptr;
}

static char* platformGetDeviceConnectStatePath(int type)
{
    static char hdmiPath[64] = {0};
    static char cvbsPath[64] = {0};
    if (type == DISP_OUTPUT_TYPE_HDMI && hdmiPath[0])
        return hdmiPath;
    if (type == DISP_OUTPUT_TYPE_TV && cvbsPath[0])
        return cvbsPath;

    struct stat extcon_fs;
    char path[64];
    char buf[32] = {0};
    for (int i = 0; i < 16; i++) {
        sprintf(path, "/sys/class/extcon/extcon%d/state", i);
            if (stat(path, &extcon_fs) == 0) {
                if (read_from_file(path, buf, 32) > 0) {
                    if (strstr(buf, "HDMI") != NULL)
                        memcpy(hdmiPath, path, strlen(path) + 1);
                    else if (strstr(buf, "CVBS") != NULL)
                        memcpy(cvbsPath, path, strlen(path) + 1);
                }
            }
        if (hdmiPath[0] != 0 && cvbsPath[0] != 0)
            goto OUT;
    }
        if (hdmiPath[0] == 0)
                memcpy(hdmiPath, "hdmi_err", 9);
        if (cvbsPath[0] == 0)
                memcpy(cvbsPath, "cvbs_err", 9);
OUT:
        dd_info("platformGetDeviceConnectStatePath:\n"
                  "    hdmi: %s\n    cvbs: %s",
                  hdmiPath, cvbsPath);
        return type == DISP_OUTPUT_TYPE_HDMI ? hdmiPath : cvbsPath;
}
int platformGetDeviceConnectState(int type)
{
    int connected = 0;
    switch (type) {
    case DISP_OUTPUT_TYPE_HDMI:
    case DISP_OUTPUT_TYPE_TV:
        connected = getConnectStateFromFile(platformGetDeviceConnectStatePath(type));
        break;
    case DISP_OUTPUT_TYPE_LCD:
        connected = 1;
        break;
    default:
        connected = -1;
    }
    if (connected < 0) {
        dd_error("platformGetDeviceConnectState failed");
        connected = 0;
    }
    return connected;
}

class DefaultEnhance : public HardwareCtrl::EnhanceBase {
public:
    int getEnhanceComponent(int option) {
        return getComponent(option);
    }
    int setEnhanceComponent(int option, int t) {
        return setComponent(option, t);
    }
    int setup() {
        setComponent(kEnhanceBright, 5);
        setComponent(kEnhanceContrast, 0);
        setComponent(kEnhanceSaturation, 4);
        setComponent(kEnhanceDenoise, 5);
        setComponent(kEnhanceDetail, 2);
        setComponent(kEnhanceMode, 1);
        return 0;
    }

private:
    const char *_prefix = "/sys/class/disp/disp/attr";
    const char *_attr_list[7] = {
        "enhance_mode",       /* kEnhanceMode       = 0 */
        "enhance_bright",     /* kEnhanceBright     = 1 */
        "enhance_contrast",   /* kEnhanceContrast   = 2 */
        "enhance_denoise",    /* kEnhanceDenoise    = 3 */
        "enhance_detail",     /* kEnhanceDetail     = 4 */
        "enhance_edge",       /* kEnhanceEdge       = 5 */
        "enhance_saturation", /* kEnhanceSaturation = 6 */
    };

    int getComponent(int option) {
        if (option >= (int)NELEM(_attr_list) || option < 0) {
            dd_error("DefaultEnhance: unknow component (%d)", option);
            return -1;
        }
        char path[128] = {0};
        sprintf(path, "%s/%s", _prefix, _attr_list[option]);
        return readIntFromFile(path);
    }

    int setComponent(int option, int t) {
        if (option >= (int)NELEM(_attr_list) || option < 0) {
            dd_error("DefaultEnhance: unknow component (%d)", option);
            return -1;
        }
        char path[128] = {0};
        sprintf(path, "%s/%s", _prefix, _attr_list[option]);
        if (t < 0)  t = 0;
        if (t > 10) t = 10;
        return writeIntToFile(path, t);
    }
};

HardwareCtrl::EnhanceBase *platformGetEnhanceHandle(int id)
{
    if (id == 0)
        return new DefaultEnhance();
    return nullptr;
}

