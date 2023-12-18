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

#include <cutils/properties.h>
#include "Debug.h"
#include "disputils.h"
#include "uniquefd.h"
#include "CompositionEngineV2Impl.h"
#include "TabletDeviceFactory.h"
#include "hardware/sunxi_display2.h"
#include "RotatorManager.h"
#include "vendorservice/default/VendorServiceAdapter.h"

namespace sunxi {

TabletDeviceFactory::TabletDeviceFactory()
    : mConnectedDisplays(),
      mListener(nullptr)
{
    VendorServiceAdapter::getService()->setVendorInterface(this);
}

TabletDeviceFactory::~TabletDeviceFactory()
{
    mConnectedDisplays.clear();
    mListener = nullptr;
}

std::vector<IVendorInterface::DeviceInfo> TabletDeviceFactory::getOutputTopology() const
{
    const IVendorInterface::DeviceInfo displayDeviceTopology[eTopologyMaxNumber][MaxDisplayCount] = {
        /* single display mapping */
        {
            {
                .logicalId     = HWC_DISPLAY_PRIMARY,
                .connected     = 1,
                .hardwareIndex = 0,
                .type          = DISP_OUTPUT_TYPE_LCD,
            },
            {
                .logicalId     = HWC_DISPLAY_EXTERNAL,
                .connected     = 0,
                .hardwareIndex = 1,
                .type          = 0,
            },
        },

        /* dual display mapping */
        {
            {
                .logicalId     = HWC_DISPLAY_PRIMARY,
                .connected     = 1,
                .hardwareIndex = 0,
                .type          = DISP_OUTPUT_TYPE_LCD,
            },
            {
                .logicalId     = HWC_DISPLAY_EXTERNAL,
                .connected     = 1,
                .hardwareIndex = 1,
                .type          = DISP_OUTPUT_TYPE_LCD,
            },
        },
    };

    std::vector<IVendorInterface::DeviceInfo> mapping;
    mapping.push_back(displayDeviceTopology[eTopologySingleScreen][0]);
    mapping.push_back(displayDeviceTopology[eTopologySingleScreen][1]);
    return mapping;
}

int TabletDeviceFactory::bringupDevices()
{
    std::vector<DeviceInfo> mapping = getOutputTopology();
    for (auto& info : mapping) {
        if (info.connected == 1) {
            if (getDisplayOutputType(info.hardwareIndex) != info.type) {
                DLOGW("display.%d was off, request type %d, try to bringup it!",
                        info.hardwareIndex, info.type);
                int ret = ::switchDisplay(info.hardwareIndex, info.type);
                DLOGI("display.%d switch to type %d, return %d",
                        info.hardwareIndex, info.type, ret);
            }
        }
    }
    return 0;
}

void TabletDeviceFactory::fixDisplaySizebyMemory(DeviceBase::Config &config)
{
    int lowmem_768m = property_get_bool("persist.vendor.lowmem.768m", 0);
    if (lowmem_768m) {
        int width = config.width;
        int height = config.height;
        float scale = 1.0, wscale, hscale;
        if (width > height) {
            wscale = 960.f / width;
            hscale = 540.f / height;
        } else {
            wscale = 960.f / height;
            hscale = 540.f / width;
        }
        scale = wscale > hscale ? hscale : wscale;
        width = (int)(width * scale) / 10 * 10;
        height = (int)(height * scale) / 10 * 10;
        config.width = width;
        config.height = height;
    }
}

void TabletDeviceFactory::scanConnectedDevice()
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);

    bool hasPrimaryDisplay = false;
    mConnectedDisplays.clear();

    bringupDevices();

    for (int hardwareIndex  = 0; hardwareIndex < MaxDisplayCount; hardwareIndex++) {
        int type = getDisplayOutputType(hardwareIndex);
        const char *typestr = outputType2Name(type);
        DLOGI("DE[%d] output type: %s(%d)", hardwareIndex, typestr, type);

        if (type == DISP_OUTPUT_TYPE_LCD) {
            DeviceBase::Config config = makeLcdDefaultConfig();
            std::string name = std::string(typestr) + std::to_string(hardwareIndex);
            int width = config.width;
            int height = config.height;
            fixDisplaySizebyMemory(config);
            auto engine = CompositionEngineV2Impl::create(hardwareIndex);

            // adjust the memory limitation according to framebuffer size.
            engine->reconfigBandwidthLimitation(config);

            auto device = std::make_shared<Disp2Device>(name, engine, config);
            device->setOutput(width, height, config.refreshRate);

            std::shared_ptr<RotatorManager> rotator = std::make_shared<RotatorManager>();
            device->setRotatorManager(rotator);

            if (!hasPrimaryDisplay) {
                ConnectedInfo c;
                c.displayId = HWC_DISPLAY_PRIMARY;  // always assume lcd as primary display
                c.connected = 1;
                c.device = device;
                c.hardwareIndex = hardwareIndex;
                mConnectedDisplays.push_back(c);
                hasPrimaryDisplay = true;
            } else {
                // override with the actual screen size
                int width = 0;
                int height = 0;
                getDisplayOutputSize(hardwareIndex, &width, &height);
                device->setOutput(width, height, config.refreshRate);

                ConnectedInfo c;
                c.displayId = HWC_DISPLAY_EXTERNAL;
                c.connected = 1;
                c.device = device;
                c.hardwareIndex = hardwareIndex;
                mConnectedDisplays.push_back(c);
            }
        } else {
            DLOGW("not support type [%s] on tablet platform yet!", typestr);
        }
    }

    if (!mListener) {
        DLOGE("DeviceChangedListener not register yet!");
        return;
    }
    mListener->onDeviceChanged(mConnectedDisplays);
}

DeviceBase::Config TabletDeviceFactory::makeLcdDefaultConfig() const
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

    double refreshRate = 1000000000000.f /
        (double(info.upper_margin + info.lower_margin + info.vsync_len + info.yres)
         * ( info.left_margin  + info.right_margin + info.hsync_len + info.xres)
         * info.pixclock);

    refreshRate = floor(refreshRate + 0.5f);

    config.refreshRate = refreshRate;
    if (config.refreshRate <= 0 || refreshRate > 600) {
        DLOGE("invalid refresh rate, assuming 60 Hz");
        config.refreshRate = 60;
    }

    DLOGD("lcd info: %dx%d %dHz", info.xres, info.yres, config.refreshRate);

    if (info.width != 0 && info.height != 0) {
        config.dpix = 1000 * (info.xres * 25.4f) / info.width;
        config.dpiy = 1000 * (info.yres * 25.4f) / info.height;
    }
    config.width  = info.xres;
    config.height = info.yres;

    return config;
}

void TabletDeviceFactory::registerDeviceChangedListener(DeviceChangedListener* listener)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    mListener = listener;
}

std::shared_ptr<DeviceBase> TabletDeviceFactory::getPrimaryDevice()
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

int TabletDeviceFactory::freezeOuput(int display, bool enabled)
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

int32_t TabletDeviceFactory::getHwIdx(int32_t logicId)
{
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == logicId) {
            return item.hardwareIndex;
        }
    }
    return -1;
}

// require by HWComposer.cpp
std::shared_ptr<IDeviceFactory> createDeviceFactory() {
    DLOGI("IDeviceFactory from %s", __FILE__);
    return std::make_shared<TabletDeviceFactory>();
}

} // namespace sunxi

