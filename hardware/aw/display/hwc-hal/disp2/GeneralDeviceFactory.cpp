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

#include "Debug.h"
#include "disputils.h"
#include "uniquefd.h"
#include "CompositionEngineV2Impl.h"
#include "GeneralDeviceFactory.h"
#include "hardware/sunxi_display2.h"
#include "RotatorManager.h"
#include "vendorservice/default/VendorServiceAdapter.h"
#include "configs/DeviceConfig.h"

namespace sunxi {

GeneralDeviceFactory::GeneralDeviceFactory()
    : mConnectedDisplays(),
      mListener(nullptr),
      mDeviceConfigInfo(new DeviceConfig())
{
    VendorServiceAdapter::getService()->setVendorInterface(this);
}

GeneralDeviceFactory::~GeneralDeviceFactory()
{
    mConnectedDisplays.clear();
    mListener = nullptr;
}

int GeneralDeviceFactory::bringupDevices()
{
    // parse device configs from file
    mDeviceConfigInfo->init();

    std::string infostr;
    mDeviceConfigInfo->dump(infostr);
    ALOGD("%s", infostr.c_str());

    const std::vector<DeviceConfig::DeviceHardwareInfo>& devlist =
        mDeviceConfigInfo->getDeviceList();

    for (const auto& info : devlist) {
        int error = 0;
        if (getDisplayOutputType(info.DisplayEnginePortId) != info.InterfaceType) {
            DLOGW("display.%d was off, request type %d, try to bringup it!",
                    info.DisplayEnginePortId, info.InterfaceType);
            error = ::switchDisplay(info.DisplayEnginePortId, info.InterfaceType);
            DLOGI("display.%d switch to type %d, return %d",
                    info.DisplayEnginePortId, info.InterfaceType, error);
        }
        if (!error)
            mHardwareDevices.push_back(info);
    }
    return 0;
}

void GeneralDeviceFactory::scanConnectedDevice()
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);

    mConnectedDisplays.clear();
    mHardwareDevices.clear();
    bringupDevices();

    for (const auto& devinfo : mHardwareDevices) {
        int hardwareIndex = devinfo.DisplayEnginePortId;
        int type = devinfo.InterfaceType;
        const char *typestr = outputType2Name(type);
        DLOGI("DE[%d] output type: %s(%d)", hardwareIndex, typestr, type);

        DeviceBase::Config config = makeConfigFromHardwareInfo(devinfo);
        std::string name = std::string(typestr) + std::to_string(hardwareIndex);
        auto engine = CompositionEngineV2Impl::create(hardwareIndex);
        auto device = std::make_shared<Disp2Device>(name, engine, config);

        // query screen size from driver and update to the compositor
        int width  = 0;
        int height = 0;
        getDisplayOutputSize(hardwareIndex, &width, &height);
        device->setOutput(width, height, config.refreshRate);

        if (devinfo.HardwareRotateSupported) {
            std::shared_ptr<RotatorManager> rotator = std::make_shared<RotatorManager>();
            device->setRotatorManager(rotator);
        }

        ConnectedInfo c;
        c.displayId = devinfo.DisplayId;
        c.connected = 1;
        c.device = device;
        c.hardwareIndex = hardwareIndex;
        mConnectedDisplays.push_back(c);
    }

    if (!mListener) {
        DLOGE("DeviceChangedListener not register yet!");
        return;
    }
    mListener->onDeviceChanged(mConnectedDisplays);
}

DeviceBase::Config GeneralDeviceFactory::makeLcdDefaultConfig() const
{
    DeviceBase::Config config = {
        .name = std::string("default-config"),
        .configId = 0,
        .width = 1280,
        .height = 720,
        .dpix = 160,
        .dpiy = 160,
        .refreshRate = 60,
    };
    struct fb_var_screeninfo info;
    if (getFramebufferVarScreenInfo(&info) != 0) {
        DLOGE("get var screeninfo failed!");
        return config;
    }

    int refreshRate = 1000000000000LLU /
        (uint64_t(info.upper_margin + info.lower_margin + info.vsync_len + info.yres)
         * ( info.left_margin  + info.right_margin + info.hsync_len + info.xres)
         * info.pixclock);
    if (refreshRate == 0) {
        DLOGE("invalid refresh rate, assuming 60 Hz");
        refreshRate = 60;
    }
    config.refreshRate = refreshRate;

    DLOGD("lcd info: %dx%d %dHz", info.xres, info.yres, refreshRate);

    if (info.width != 0 && info.height != 0) {
        config.dpix = 1000 * (info.xres * 25.4f) / info.width;
        config.dpiy = 1000 * (info.yres * 25.4f) / info.height;
    }
    config.width  = info.xres;
    config.height = info.yres;

    return config;
}

DeviceBase::Config GeneralDeviceFactory::makeConfigFromHardwareInfo(
        const DeviceConfig::DeviceHardwareInfo& info)
{
    DeviceBase::Config config = {
        .name = std::string("default-config"),
        .configId = 0,
        .width = 1280,
        .height = 720,
        .dpix = 160,
        .dpiy = 160,
        .refreshRate = 60,
    };

    if (info.InterfaceType == DISP_OUTPUT_TYPE_LCD &&
            info.DisplayEnginePortId == 0) {
        config = makeLcdDefaultConfig();
    }

    if (info.OverrideFramebufferSize &&
            info.FramebufferWidth != 0 && info.FramebufferHeight != 0) {
        config.width  = info.FramebufferWidth;
        config.height = info.FramebufferHeight;
        config.dpix = info.DpiX * 1000;
        config.dpiy = info.DpiY * 1000;
    }

    return config;
}

void GeneralDeviceFactory::registerDeviceChangedListener(DeviceChangedListener* listener)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    mListener = listener;
}

std::shared_ptr<DeviceBase> GeneralDeviceFactory::getPrimaryDevice()
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = nullptr;
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == HWC_DISPLAY_PRIMARY) {
            device = item.device;
            break;
        }
    }
    return device;
}

int GeneralDeviceFactory::freezeOuput(int display, bool enabled)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = nullptr;
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == HWC_DISPLAY_PRIMARY) {
            device = item.device;
            break;
        }
    }

    if (device != nullptr) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->freezeOuput(enabled);
        return 0;
    }
    DLOGW("Can't fine display %d", display);
    return -ENODEV;
}

std::shared_ptr<DeviceBase> GeneralDeviceFactory::findDevice(int display)
{
    std::shared_ptr<DeviceBase> device = nullptr;
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == display) {
            device = item.device;
        }
    }
    return device;
}

int32_t getHwIdx(int32_t logicId)
{
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == logicId) {
            return item.hardwareIndex;
        }
    }
    return -1;
}

// require by HWComposer.cpp
std::shared_ptr<IDeviceFactory> createDeviceFactory()
{
    DLOGI("IDeviceFactory from %s", __FILE__);
    return std::make_shared<GeneralDeviceFactory>();
}

int GeneralDeviceFactory::setDeviceOutputInfo(
        int display, int xres, int yres, int refreshRate)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->setOutput(xres, yres, refreshRate);
    }
    return 0;
}

} // namespace sunxi

