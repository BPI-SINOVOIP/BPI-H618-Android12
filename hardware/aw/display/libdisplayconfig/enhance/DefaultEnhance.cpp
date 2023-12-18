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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <cutils/properties.h>
#include <utils/Log.h>
#include "EnhanceBase.h"
#include "hardware/sunxi_display2.h"

namespace sunxi {

class DefaultEnhance : public EnhanceBase {
public:
    int setup() override;
    int getEnhanceComponent(int option) override;
    int setEnhanceComponent(int option, int t) override;
    int setSmartBackLight(int mode) override;

private:
    static const int mMaxAttrCount = 8;
    const char *prefix_ = "/sys/class/disp/disp/attr";
    const char *attr_list_[mMaxAttrCount] = {
        "enhance_mode",       /* kEnhanceMode       = 0 */
        "enhance_bright",     /* kEnhanceBright     = 1 */
        "enhance_contrast",   /* kEnhanceContrast   = 2 */
        "enhance_denoise",    /* kEnhanceDenoise    = 3 */
        "enhance_detail",     /* kEnhanceDetail     = 4 */
        "enhance_edge",       /* kEnhanceEdge       = 5 */
        "enhance_saturation", /* kEnhanceSaturation = 6 */
        "color_temperature",  /* kColorTemperature  = 7 */
    };

    int getComponent(int option);
    int setComponent(int option, int t);
};

int DefaultEnhance::setup()
{
    return 0;
}

int DefaultEnhance::getEnhanceComponent(int option)
{
    int value = getComponent(option);
    switch (option) {
        case kEnhanceMode:
            if (value == 3) {
                //disp2 driver assume the 3 as the demo mode!
                value = kEnhanceModeDemo;
            }
            break;
        default:
            break;
    }
    return value;
}

static const int COLOR_TEMPERATURE_STRENGTH_OFFSET = 50;

int DefaultEnhance::setEnhanceComponent(int option, int t)
{
    switch (option) {
        case kEnhanceMode:
            if (t < 0 || t > 3) {
                return -1;
            } else if (t == kEnhanceModeDemo) {
                //disp2 driver assume the 3 as the demo mode!
                t = 3;
            }
            break;
        case kColorTemperature:
            t -= COLOR_TEMPERATURE_STRENGTH_OFFSET;
            break;
        default:
            if (t < 0 || t > 100) return -1;
    }
    return setComponent(option, t);
}

int DefaultEnhance::getComponent(int option) {
    if (option >= mMaxAttrCount || option < 0) {
        ALOGE("DefaultEnhance: unknow component (%d)", option);
        return -1;
    }
    char path[128] = {0};
    sprintf(path, "%s/%s", prefix_, attr_list_[option]);
    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        ALOGE("Could not open '%s', %s", path, strerror(errno));
        return -errno;
    }
    char buf[32] = {0};
    read(fd, buf, sizeof(buf) - 1);
    close(fd);
    return atoi(buf);
}

int DefaultEnhance::setComponent(int option, int t) {
    if (option >= mMaxAttrCount || option < 0) {
        ALOGE("DefaultEnhance: unknow component (%d)", option);
        return -1;
    }
    char path[128] = {0};
    sprintf(path, "%s/%s", prefix_, attr_list_[option]);
    int fd = open(path, O_WRONLY, 0);
    if (fd < 0) {
        ALOGE("Could not open '%s', %s", path, strerror(errno));
        return -1;
    }
    char buf[16] = {0}; sprintf(buf, "%d", t);
    write(fd, buf, strlen(buf));
    close(fd);
    return 0;
}

int DefaultEnhance::setSmartBackLight(int mode)
{
    int fd = open("/dev/disp", O_RDWR);
    if (fd < 0) {
        ALOGE("Could not open disp device: %s", strerror(errno));
        return -1;
    }

    int ret = 0;
    unsigned long args[4] = {0};
    if (mode == kSmartBackLightOff) {
        args[0] = 0; // disp id, only DE0 support smart backlight
        ret = ioctl(fd, DISP_SMBL_DISABLE, args);
    } else if (mode == kSmartBackLightOn || mode == kSmartBackLightDemo) {
        args[0] = 0; // disp id, only DE0 support smart backlight
        int width  = ioctl(fd, DISP_GET_SCN_WIDTH,  args);
        int height = ioctl(fd, DISP_GET_SCN_HEIGHT, args);

        struct disp_rect window;
        window.x = 0;
        window.y = 0;

        if (width > height) {
            window.width  = (mode == kSmartBackLightDemo) ?  width/2 : width;
            window.height = height;
        } else {
            window.width = width;
            window.height = (mode == kSmartBackLightDemo) ?  height/2 : height;
        }

        args[1] = (unsigned long)&window;
        ret = ioctl(fd, DISP_SMBL_SET_WINDOW, args);
        ret = ioctl(fd, DISP_SMBL_ENABLE, args);
    }
    close(fd);
    return ret;
}

std::unique_ptr<EnhanceBase> createEnhanceHandle() {
    std::unique_ptr<DefaultEnhance> enhance = std::make_unique<DefaultEnhance>();
    return enhance;
}

} // namespace sunxi

