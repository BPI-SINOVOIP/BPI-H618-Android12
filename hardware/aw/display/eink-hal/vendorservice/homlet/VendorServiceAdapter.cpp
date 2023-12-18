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

#include <hardware/hwcomposer.h>
#include "Debug.h"
#include "hardware/sunxi_display2.h"
#include "VendorServiceAdapter.h"
#include "IVendorInterface.h"

namespace sunxi {

struct SunxiModeInfo {
    int mode;
    int xres;
    int yres;
    int refreshRate;
};
static const SunxiModeInfo _modeInfos[] = {
    {DISP_TV_MOD_NTSC,              720,  480, 60},
    {DISP_TV_MOD_PAL,               720,  576, 50},

    {DISP_TV_MOD_480I,              720,  480, 60},
    {DISP_TV_MOD_576I,              720,  576, 60},
    {DISP_TV_MOD_480P,              720,  480, 60},
    {DISP_TV_MOD_576P,              720,  576, 50},
    {DISP_TV_MOD_720P_50HZ,        1280,  720, 50},
    {DISP_TV_MOD_720P_60HZ,        1280,  720, 60},

    {DISP_TV_MOD_1080P_24HZ,       1920, 1080, 24},
    {DISP_TV_MOD_1080P_25HZ,       1920, 1080, 25},
    {DISP_TV_MOD_1080P_30HZ,       1920, 1080, 30},
    {DISP_TV_MOD_1080P_50HZ,       1920, 1080, 50},
    {DISP_TV_MOD_1080P_60HZ,       1920, 1080, 60},
    {DISP_TV_MOD_1080I_50HZ,       1920, 1080, 50},
    {DISP_TV_MOD_1080I_60HZ,       1920, 1080, 60},

    {DISP_TV_MOD_3840_2160P_25HZ,  3840, 2160, 25},
    {DISP_TV_MOD_3840_2160P_24HZ,  3840, 2160, 24},
    {DISP_TV_MOD_3840_2160P_30HZ,  3840, 2160, 30},
    {DISP_TV_MOD_3840_2160P_50HZ,  3840, 2160, 50},
    {DISP_TV_MOD_3840_2160P_60HZ,  3840, 2160, 60},

    {DISP_TV_MOD_4096_2160P_24HZ,  4096, 2160, 24},
    {DISP_TV_MOD_4096_2160P_25HZ,  4096, 2160, 25},
    {DISP_TV_MOD_4096_2160P_30HZ,  4096, 2160, 30},
    {DISP_TV_MOD_4096_2160P_50HZ,  4096, 2160, 50},
    {DISP_TV_MOD_4096_2160P_60HZ,  4096, 2160, 60},

    {DISP_TV_MOD_1080P_24HZ_3D_FP, 1920, 1080, 24},
    {DISP_TV_MOD_720P_50HZ_3D_FP,  1280,  720, 50},
    {DISP_TV_MOD_720P_60HZ_3D_FP,  1280,  720, 60},

    {-1, -1, -1, -1},
};

class VendorModeInfo {
public:
    explicit VendorModeInfo(int mode) {
        info = &_modeInfos[0];
        for (int i = 0; _modeInfos[i].mode != -1; i++) {
            if (_modeInfos[i].mode == mode) {
                info = &_modeInfos[i];
                break;
            }
        }
    }
    int xres() const { return info->xres; }
    int yres() const { return info->yres; }
    int refreshRate() const { return info->refreshRate; }
private:
    const SunxiModeInfo* info;
};

using DeviceInfo = IVendorInterface::DeviceInfo;

VendorServiceAdapter::VendorServiceAdapter()
    : mInterface(nullptr), mEventCallback(nullptr)
{
    initialize();
}

VendorServiceAdapter* VendorServiceAdapter::getService()
{
    static VendorServiceAdapter _instance;
    return &_instance;
}

void VendorServiceAdapter::initialize()
{
    /* Register vendor service: DisplayConfigService */
    mDisplayConfigService = new sunxi::DisplayConfigService(*this);
    android::status_t status = mDisplayConfigService->publish();
    DLOGI("DisplayConfigService init: %s", (status == android::OK) ? "ok" : "error");
}

void VendorServiceAdapter::setVendorInterface(IVendorInterface* intf)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
    mInterface = intf;
    if (mInterface) {
        updateConnectedDevicesInternal();
    }
    DLOGI("register IVendorInterface: %p", intf);
}

int VendorServiceAdapter::setDisplayArgs(int display, int cmd1, int cmd2, int data)
{
    // legacy api, should not use in hwc3.0
    return 0;
}

int VendorServiceAdapter::blank(int display, int enable)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
    if (mInterface)
        return mInterface->blankDevice(display, !!enable);
    return 0;
}

// TODO: Get hardware id from configs
static int outputIntfToHardwareId(int type)
{
    switch (type) {
        case DISP_OUTPUT_TYPE_HDMI: return 0;
        case DISP_OUTPUT_TYPE_TV:   return 1;
        default: return -1;
    }
}

int VendorServiceAdapter::switchDevice(const DeviceTable& tables)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);

    for (size_t i = 0; i < tables.mTables.size(); i++) {
        const DeviceTable::device_info_t& devinfo = tables.mTables[i];
        DLOGI("display[%d]: type=%d mode=%d enable=%d",
                devinfo.logicalId, devinfo.type, devinfo.mode, devinfo.enabled);

        if (mDeviceList.count(devinfo.logicalId) == 0) {
            std::unique_ptr<DeviceInfo> newinfo(new DeviceInfo());
            mDeviceList.emplace(devinfo.logicalId, std::move(newinfo));
        }

        std::unique_ptr<DeviceInfo>& device = mDeviceList[devinfo.logicalId];
        VendorModeInfo info(devinfo.mode);
        device->logicalId = devinfo.logicalId;
        device->connected = devinfo.enabled;
        device->hardwareIndex = outputIntfToHardwareId(devinfo.type);
        device->type = devinfo.type;
        device->xres = info.xres();
        device->yres = info.yres();
        device->refreshRate = info.refreshRate();
    }

    return updateConnectedDevicesInternal();
}

int VendorServiceAdapter::updateConnectedDevicesInternal()
{
    if (!mInterface)
        return 0;

    std::vector<DeviceInfo *> connectedDevices;
    for (auto& item : mDeviceList) {
        DeviceInfo *device = item.second.get();
        connectedDevices.push_back(device);
    }
    return mInterface->updateConnectedDevices(connectedDevices);
}

int VendorServiceAdapter::setOutputMode(int display, int type, int mode)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
    if (!mInterface)
        return 0;

    if (mDeviceList.count(display)) {
        std::unique_ptr<DeviceInfo>& device = mDeviceList[display];
        VendorModeInfo info(mode);
        device->xres = info.xres();
        device->yres = info.yres();
        device->refreshRate = info.refreshRate();

        return mInterface->setDeviceOutputInfo(
                display, device->xres, device->yres, device->refreshRate);
    }
    return 0;
}

int VendorServiceAdapter::setMargin(int display, int l, int r, int t, int b)
{
    return 0;
}

int VendorServiceAdapter::setVideoRatio(int display, int ratio)
{
    return 0;
}

int VendorServiceAdapter::set3DMode(int display, int mode)
{
    return 0;
}

int VendorServiceAdapter::setDataspace(int display, int dataspace)
{
    return 0;
}

int VendorServiceAdapter::registerCallback(IHWCPrivateService::EventCallback* cb)
{
    mEventCallback = cb;
    return 0;
}

void VendorServiceAdapter::setDebugTag(int32_t tag)
{

}

SNRApi *VendorServiceAdapter::getSNRInterface() {
    return nullptr;
}

}
