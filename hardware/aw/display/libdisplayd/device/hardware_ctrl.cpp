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

#include <fcntl.h>
#include "platform.h"
#include "device/hardware_ctrl.h"

HardwareCtrl::HardwareCtrl(int hardwareId)
  : mHardwareId(hardwareId),
    mEnhance(nullptr),
    mDispFd(-1)
{
    mDispFd = open("/dev/disp", O_RDWR);
    if (mDispFd <= 0) {
        dd_error("Open /dev/disp failed, %s", strerror(errno));
        return;
    }

    mEnhance = platformGetEnhanceHandle(mHardwareId);
    if (mEnhance) {
        mEnhance->setup();
    }
}

HardwareCtrl::~HardwareCtrl()
{
    close(mDispFd);
    mDispFd = -1;
}

int HardwareCtrl::getMode()
{
    disp_output result;
    dispioctl(DISP_GET_OUTPUT, (unsigned long)&result);
    return result.mode;
}

int HardwareCtrl::setMode(int mode)
{
    disp_output result;
    dispioctl(DISP_GET_OUTPUT, (unsigned long)&result);
    return dispioctl(DISP_DEVICE_SWITCH, result.type, mode);
}

int HardwareCtrl::checkModeSupport(int mode)
{
    return dispioctl(DISP_HDMI_SUPPORT_MODE, mode);
}

int HardwareCtrl::getDeviceConfig(struct disp_device_config *config)
{
    if (config == nullptr) {
        dd_error("getDeviceConfig: error input param");
        return -1;
    }
    return dispioctl(DISP_DEVICE_GET_CONFIG, (unsigned long)config);
}

int HardwareCtrl::setDeviceConfig(struct disp_device_config *config)
{
    if (config == nullptr) {
        dd_error("getDeviceConfig: error input param");
        return -1;
    }
    return dispioctl(DISP_DEVICE_SET_CONFIG, (unsigned long)config);
}

int HardwareCtrl::dispioctl(int request, unsigned long arg1,
                            unsigned long arg2, unsigned long arg3) const
{
    unsigned long args[4] = {
        static_cast<unsigned long>(mHardwareId), arg1, arg2, arg3};
    return ioctl(mDispFd, request, args);
}
