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
#include "displayd/device_manager.h"

#include "CompositionEngineV2Impl.h"
#include "Debug.h"
#include "disputils.h"
#include "hardware/sunxi_display2.h"
#include "HomletDeviceFactory.h"
#include "vendorservice/homlet/VendorServiceAdapter.h"
#include "WriteBackManager.h"
#include <cutils/properties.h>

namespace sunxi {

HomletDeviceFactory::HomletDeviceFactory()
  : mConnectedDisplays(),
    mListener(nullptr),
    mDeviceConfigInfo(new DeviceConfig())
{
    mDeviceConfigInfo->init();
    VendorServiceAdapter::getService()->setVendorInterface(this);
}

HomletDeviceFactory::~HomletDeviceFactory()
{
    VendorServiceAdapter::getService()->setVendorInterface(nullptr);
    mConnectedDisplays.clear();
    mHardwareDevices.clear();
    mListener = nullptr;
    mDisplaydCb = nullptr;
}

int HomletDeviceFactory::bringupDevices()
{
    std::string infostr;
    mDeviceConfigInfo->dump(infostr);
    ALOGD("%s", infostr.c_str());

    const std::vector<DeviceConfig::DeviceHardwareInfo>& devlist =
        mDeviceConfigInfo->getDeviceList();

    for (const auto& info : devlist) {
        if (getDisplayOutputType(info.DisplayEnginePortId) != info.InterfaceType) {
            DLOGW("display.%d was off, request type %d, try to bringup it!",
                    info.DisplayEnginePortId, info.InterfaceType);
            /*error = ::switchDisplay(info.DisplayEnginePortId, info.InterfaceType);
            DLOGI("display.%d switch to type %d, return %d",
                    info.DisplayEnginePortId, info.InterfaceType, error);*/
        }
    }
    return 0;
}

DeviceBase::Config HomletDeviceFactory::makeLcdDefaultConfig() const
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

#if 0
struct HdmiModeInfo {
    int mode;
    int xres;
    int yres;
    int dpix;
    int dpiy;
    int refreshRate;
};

static const HdmiModeInfo _modeInfos[] = {
    {DISP_TV_MOD_800_480P_60HZ,      800, 480, 160, 160, 60},
    {DISP_TV_MOD_1024_600P_60HZ,     1024, 600, 160, 160, 60},
    {DISP_TV_MOD_1440_2560P_60HZ,    1440, 2560, 160, 160, 60},

    {DISP_TV_MOD_480P,               720, 480, 160, 160, 60},
    {DISP_TV_MOD_720P_60HZ,          1280, 720, 160, 160, 60},
    {DISP_TV_MOD_1080P_60HZ,         1920, 1080, 160, 160, 60},
    {DISP_TV_MOD_3840_2160P_30HZ,    3840, 2160, 160, 160, 30},
    {-1, -1, -1, -1, -1, -1},
};
#endif

DeviceBase::Config HomletDeviceFactory::makeConfigFromHardwareInfo(int id)
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

    const std::vector<DeviceConfig::DeviceHardwareInfo>& devlist =
        mDeviceConfigInfo->getDeviceList();

    if (id < 0 || id >= devlist.size()) {
        return config;
    }

    DeviceConfig::DeviceHardwareInfo info = devlist[id];
    if (info.InterfaceType == DISP_OUTPUT_TYPE_LCD &&
            info.DisplayEnginePortId == 0) {
        config = makeLcdDefaultConfig();
    }

#if 0
    if (info.InterfaceType == DISP_OUTPUT_TYPE_HDMI &&
	    info.DisplayEnginePortId == 0) {
	//bpi, get save propery and map to real x,y
	char property[PROPERTY_VALUE_MAX] = {0};
	if (0 >= property_get(HDMI_USER_STRATEGY, property, NULL)) {
            DLOGE("BPI: makeConfigFromHardwareInfo(), property_get '%s' failed", HDMI_USER_STRATEGY);
        } else {
	        int strategy = atoi(property);
	        DLOGE("BPI: makeConfigFromHardwareInfo(), hdmi user strategy: %d", strategy);

	        for (int i = 0; _modeInfos[i].mode != -1; i++) {
	            if (_modeInfos[i].mode == strategy) {
	                config.width  = _modeInfos[i].xres;
	                config.height = _modeInfos[i].yres;
	                config.dpix = _modeInfos[i].dpix;
	                config.dpix = _modeInfos[i].dpiy;
		      DLOGE("BPI: makeConfigFromHardwareInfo(), width=%d height=%d", config.width, config.height);
	                break;
	            }
		}
	}
    }
#endif

    if (info.OverrideFramebufferSize &&
            info.FramebufferWidth != 0 && info.FramebufferHeight != 0) {
        config.width  = info.FramebufferWidth;
        config.height = info.FramebufferHeight;
        config.dpix = info.DpiX;
        config.dpiy = info.DpiY;
    }

    return config;
}

void HomletDeviceFactory::scanConnectedDevice()
{
    std::unique_lock<std::mutex> lock(mDeviceMutex);
    bringupDevices();
    if (mConnectedDisplays.empty()) {
        DLOGI("wait for device connect");
        mCondition.wait(lock);
    }
    notifyDeviceChanged();
}

void HomletDeviceFactory::notifyDeviceChanged()
{
    if (!mListener) {
        DLOGE("DeviceChangedListener not register yet!");
        return;
    }
    mListener->onDeviceChanged(mConnectedDisplays);
}

std::shared_ptr<DeviceBase> HomletDeviceFactory::findDevice(int display)
{
    std::shared_ptr<DeviceBase> device = nullptr;
    for (auto& item : mConnectedDisplays) {
        if (item.displayId == display) {
            device = item.device;
        }
    }
    return device;
}

void HomletDeviceFactory::registerDeviceChangedListener(DeviceChangedListener* listener)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    mListener = listener;
}

std::shared_ptr<DeviceBase> HomletDeviceFactory::getPrimaryDevice()
{
    std::unique_lock<std::mutex> lock(mDeviceMutex);
    if (mConnectedDisplays.empty()) {
        DLOGI("wait for device connect");
        mCondition.wait(lock);
    }
    return findDevice(HWC_DISPLAY_PRIMARY);
}

void HomletDeviceFactory::sortByDisplayId(std::vector<DeviceInfo *>& devices)
{
    std::sort(devices.begin(), devices.end(),
            [](DeviceInfo* a, DeviceInfo* b){ return a->logicalId < b->logicalId; });
}

int HomletDeviceFactory::updateConnectedDevices(std::vector<DeviceInfo *>& devices)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
#ifdef WRITE_BACK_MODE
    std::shared_ptr<WriteBackManager> wbManger = WriteBackManager::getWbInstance();
    bool hasWbIdx = false;

    wbManger->setHotplugState(true);
#endif
    /*const DeviceBase::Config config = {
        .name = std::string("default-config"),
        .configId = 0,
        .width = 1280,
        .height = 720,
        .dpix = 213,
        .dpiy = 213,
        .refreshRate = 60,
    };*/

    mConnectedDisplays.clear();
    sortByDisplayId(devices);

    for (const auto devinfo : devices) {
        if (devinfo == nullptr) {
            DLOGE("DeviceInfo should not be nullptr!");
            continue;
        }

        int hardwareIndex = devinfo->hardwareIndex;
        std::shared_ptr<Disp2Device> device = nullptr;
        if (devinfo->connected) {
            if (mHardwareDevices.count(hardwareIndex)) {
                device = mHardwareDevices[hardwareIndex];
                device->setOutput(devinfo->xres, devinfo->yres, devinfo->refreshRate);
            } else {
                std::string name = std::string(outputType2Name(devinfo->type))
                    + std::to_string(hardwareIndex);
                std::shared_ptr<CompositionEngineV2Impl> engine
                    = CompositionEngineV2Impl::create(hardwareIndex);
                DeviceBase::Config config = makeConfigFromHardwareInfo(hardwareIndex);
                device = std::make_shared<Disp2Device>(name, engine, config);
                device->setOutput(devinfo->xres, devinfo->yres, devinfo->refreshRate);
                device->setDisplaydCb(mDisplaydCb);
                mHardwareDevices.emplace(hardwareIndex, device);
            }
        } else {
            if (mHardwareDevices.count(hardwareIndex)) {
                device = mHardwareDevices[hardwareIndex];
                mHardwareDevices.erase(hardwareIndex);
            }
        }
        ConnectedInfo c;
        c.displayId = devinfo->logicalId;
        c.connected = devinfo->connected;
        c.hardwareIndex = devinfo->hardwareIndex;
        c.device = device;
        mConnectedDisplays.push_back(c);

#ifdef WRITE_BACK_MODE
        if (wbManger->isSupportWb() && wbManger->getWriteBackId() == hardwareIndex) {
            hasWbIdx = true;
            ConnectedInfo ci;
            std::shared_ptr<Disp2Device> dev = nullptr;
            if (mHardwareDevices.count(WBID)) {
                dev = mHardwareDevices[WBID];
            }
            if (devinfo->connected) {
                if (dev != nullptr) {
                    ci.displayId = WBID;
                    ci.connected = 0;
                    ci.hardwareIndex = hardwareIndex;
                    ci.device = dev;
                    mConnectedDisplays.push_back(ci);
                }
                DLOGD("wb device %p plugout! size=%d", dev.get(), mConnectedDisplays.size());
            } else {
                if (dev == nullptr || dev.get() == nullptr) {
                    std::string name = std::string("wb") + std::to_string(hardwareIndex);
                    std::shared_ptr<CompositionEngineV2Impl> engine
                        = CompositionEngineV2Impl::create(hardwareIndex);
                    DeviceBase::Config config = makeConfigFromHardwareInfo(hardwareIndex);
                    dev = std::make_shared<Disp2Device>(name, engine, config);
                    dev->setDisplaydCb(mDisplaydCb);
                    dev->setMargin(100, 100, 100, 100);
                }
                dev->setOutput(SCWD, SCHG, SCRATE);
                ci.displayId = WBID;
                ci.connected = 1;
                ci.hardwareIndex = hardwareIndex;
                ci.device = dev;
                mHardwareDevices.emplace(WBID, dev);
                mConnectedDisplays.push_back(ci);
                DLOGD("wb device %p plugin!size %d", dev.get(), mConnectedDisplays.size());
            }
        }
#endif
        DLOGI("display[%d] connected=%d type=%d device=%p",
                devinfo->logicalId, devinfo->connected, devinfo->type, device.get());
    }

    mCondition.notify_all();
    notifyDeviceChanged();

    // we had finished, clear the disconnected devcie
    for (auto& display : mConnectedDisplays) {
        if (display.connected == 0) {
            display.device = nullptr;
        }
    }
    return 0;
}

int HomletDeviceFactory::blankDevice(int display, bool enabled)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->setActive(!enabled);
    }
    return 0;
}

int HomletDeviceFactory::setDeviceOutputInfo(
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

int HomletDeviceFactory::setDeviceOutputMargin(int display, int l, int r, int t, int b)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->setMargin(l, r, t, b);
    }
    return 0;
}

int HomletDeviceFactory::setDataSpace(int display, int dataspace)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->setDataSpace(dataspace);
    }
    return 0;
}

int HomletDeviceFactory::setDisplaydCb(void* cb)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    for (auto& item : mConnectedDisplays) {
        if (item.device) {
            Disp2Device *disp2dev = static_cast<Disp2Device *>(item.device.get());
            disp2dev->setDisplaydCb(cb);
        }
    }
    mDisplaydCb = cb;
    return 0;
}

int HomletDeviceFactory::set3DMode(int display, int mode)
{
    std::lock_guard<std::mutex> lock(mDeviceMutex);
    std::shared_ptr<DeviceBase> device = findDevice(display);
    if (device) {
        Disp2Device *disp2dev = static_cast<Disp2Device *>(device.get());
        disp2dev->set3DMode(mode);
    }
    return 0;
}

int HomletDeviceFactory::freezeOuput(int display, bool enabled)
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

int HomletDeviceFactory::setDeviceLayerMode(int display, int mode)
{
    return 0;
}

int32_t HomletDeviceFactory::getHwIdx(int32_t logicId)
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
    return std::make_shared<HomletDeviceFactory>();
}

} // namespace sunxi
