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

#include "displayd/device_manager.h"

#include "debug.h"
#include "IHWCPrivateService.h"
#include "platform.h"
#include "hotplug_monitor.h"
#include "output_policy.h"
#include "device/device_controler.h"
#include "mq_thread.h"
#include "hdcp/HdcpManager.h"
#include "device/device_hdmi.h"
#include "configs/DeviceConfig.h"

#include <cutils/properties.h>

class DeviceManager::HotplugCallback : public HotplugMonitor::Callback {
public:
    HotplugCallback(DeviceManager &manager) : mManager(manager) {
        hotplug = hotplugHook;
    }

    static void hotplugHook(const HotplugMonitor::Callback *cb,
                            int type, int connected) {
        auto callback = static_cast<const HotplugCallback *>(cb);
        callback->mManager.hotplug(type, connected);
    }

private:
    DeviceManager& mManager;
};

class DeviceManager::HWCCallback: public sunxi::IHWCPrivateService::EventCallback {
public:
    HWCCallback(DeviceManager& dm) : mDeviceManager(dm) { }
   ~HWCCallback() { };

private:
   virtual void onDataspaceChanged(int dataspace) {
        std::unique_lock<std::mutex> lock(mLock);
        mDeviceManager.onDataspaceChange(dataspace);
    }

    std::mutex mLock;
    DeviceManager& mDeviceManager;
};

DeviceManager::DeviceManager(sunxi::IHWCPrivateService& client)
  : mHotplugCallback(nullptr), mClient(client), mHdcpManager(nullptr),
    mDeviceConfig(nullptr)
{
    // MessageQueue thread initialize
    mMessageThread = new MessageQueueThread();
    mMessageThread->run("MQ-Thread");

    class StartupMsg: public MessageBase {
        DeviceManager *manager;
    public:
        StartupMsg(DeviceManager *m) : manager(m) { }
        virtual bool handler() {
            manager->threadedStartup();
            return true;
        }
    };

    android::sp<MessageBase> msg = new StartupMsg(this);
    mMessageThread->postMessage(msg);
}

DeviceManager::~DeviceManager() { }

using DeviceHardwareInfo = sunxi::DeviceConfig::DeviceHardwareInfo;

void DeviceManager::threadedStartup()
{
    dd_info("threadedStartup begin");
    initialize();
    mHotplugCallback = new HotplugCallback(*this);
    mHotplugMonitor =
        new HotplugMonitor(static_cast<const HotplugMonitor::Callback *>(mHotplugCallback),
                HotplugMonitor::createUeventSocket());
    dd_info("threadedStartup end");
}

void DeviceManager::initialize()
{
    mDeviceConfig = std::make_unique<sunxi::DeviceConfig>();
    mDeviceConfig->init();
    std::string info;
    mDeviceConfig->dump(info);
    dd_info("%s", info.c_str());

#if 1
    std::vector<OutputDeviceInfo> perfectMapping;
    const std::vector<DeviceHardwareInfo>& devices = mDeviceConfig->getDeviceList();
    for (size_t i = 0; i < devices.size(); i++) {
        const DeviceHardwareInfo& devinfo = devices[i];
        perfectMapping.push_back(OutputDeviceInfo{devinfo.DisplayId, devinfo.InterfaceType, 0, 0});
    }
#else
    std::vector<OutputDeviceInfo> perfectMapping;
    perfectMapping.push_back(OutputDeviceInfo{0, DISP_OUTPUT_TYPE_HDMI, 0, 0});
    perfectMapping.push_back(OutputDeviceInfo{1, DISP_OUTPUT_TYPE_TV, 0, 0});
#endif

    mPolicy = createOutputPolicy(perfectMapping);

    for (size_t i = 0; i < devices.size(); i++) {
        const DeviceHardwareInfo& devinfo = devices[i];
        DeviceControler* device = createDevice(
                                        devinfo.DisplayEnginePortId,
                                        devinfo.InterfaceType,
                                        mClient);
        if (device) {
            mDevices.push_back(device);
        }
    }

#if 0
    /* FIXME: just for debug */
    DeviceControler *controler = getControler(PRIMARY_DEVICE);
    controler->setLogicalId(0);
    mDisplayDevices.clear();
    mDisplayDevices.emplace(0, controler);
#else
    /* Trigger a hotplug to rebuild device mapping */
    dd_info("Initizlize display device mapping");
    setupDefaultOutput();

    /* Register callback for dataspace change notify */
    mHwcCallback = new HWCCallback(*this);
    mClient.registerCallback(mHwcCallback);
#endif
}

static void dumpDeviceMapping(sunxi::DeviceTable& map)
{
    char outbuf[256] = {0};
    char *p = outbuf;
    p += sprintf(p, "Current device mapping:\n");
    for (auto& t : map.mTables) {
        p += sprintf(p, "  + display [%d]: enable=%d type=%d mode=%d\n",
                     t.logicalId, t.enabled, t.type, t.mode);
    }
    dd_info("%s", outbuf);
}

void DeviceManager::setupDefaultOutput()
{
    std::unique_lock<std::mutex> lock(mLock);

    /* Force a rebuild to get current output mapping. */
    std::vector<OutputDeviceInfo> mapping;
    mPolicy->updateConnectState(-1, -1);
    mPolicy->getOutputDeviceMapping(mapping);

    sunxi::DeviceTable routing;
    mDisplayDevices.clear();
    for (size_t i = 0; i < mapping.size(); i++) {
        int type = mapping[i].type;
        int id   = mapping[i].logicalId;

        DeviceControler *controler = getControler(type);
        if (!controler) {
            dd_error("Skip unknow device, type=%d", type);
            continue;
        }
        controler->setLogicalId(id);
        if (type == DISP_OUTPUT_TYPE_HDMI) {
	    //bpi, set real mode property
            int strategy = controler->getDisplayMode();
            dd_info("setupDefaultOutput: hdmi user strategy: %d", strategy);
	    setHdmiUserSetting(strategy);
            controler->setHdmiUserSetting(strategy);
        }
        controler->performDefaultConfig(mapping[i].enabled);
        mDisplayDevices.emplace(id, controler);

        routing.mTables.push_back(
                sunxi::DeviceTable::device_info(
                    id, controler->type(), controler->getDisplayMode(),
                    mapping[i].enabled));
    }

#if 1
    if (mClient.switchDevice(routing) != 0) {
        dd_error("SwitchDevice notify return error");
    }

    /* Unblank and present */
    for (const auto& device : mDisplayDevices) {
        device.second->present();
    }
#endif

    dumpDeviceMapping(routing);
}

inline DeviceControler *DeviceManager::getControler(int type)
{
    for (auto iter = mDevices.begin();
              iter != mDevices.end(); ++iter) {
        if ((*iter)->type() == type)
            return *iter;
    }
    dd_error("Cant't fine device controler (type=%d)", type);
    return nullptr;
}

inline DeviceControler *DeviceManager::getControlerById(int id)
{
    if (mDisplayDevices.count(id) != 1) {
        dd_error("Cant't fine device controler (id=%d)", id);
        return nullptr;
    }
    return mDisplayDevices[id];
}

void DeviceManager::hotplug(int type, int connected)
{
    std::unique_lock<std::mutex> lock(mLock);

    std::vector<OutputDeviceInfo> mapping;

    /* 1. Trigger outputPolicy to rebuild device mapping */
    mPolicy->updateConnectState(type, connected);
    mPolicy->getOutputDeviceMapping(mapping);

    /* 2. Update logical device map */
    sunxi::DeviceTable routing;
    mDisplayDevices.clear();
    for (size_t i = 0; i < mapping.size(); i++) {
        int type = mapping[i].type;
        int id   = mapping[i].logicalId;

        DeviceControler *controler = getControler(type);
        if (!controler) {
            dd_error("Skip unknow device, type=%d", type);
            continue;
        }
        controler->setLogicalId(id);
        controler->performOptimalConfig(mapping[i].enabled);
        mDisplayDevices.emplace(id, controler);

        routing.mTables.push_back(
                sunxi::DeviceTable::device_info(
                    id, controler->type(), controler->getDisplayMode(),
                    mapping[i].enabled));
    }

    /* 3. Notify hwc adapter service */
    if (mClient.switchDevice(routing) != 0) {
        dd_error("SwitchDevice notify return error");
    }

    /* 4. Unblank and present/setup margin */
    for (const auto& device : mDisplayDevices) {
        device.second->present();
        device.second->onDeviceReconnected();
    }

    /* 5. debug output */
    dumpDeviceMapping(routing);
}

/*
 * Interface for displayd client.
 * Protect by std::mutex mLock
 */

int DeviceManager::getType(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->type();
    return -1;
}

int DeviceManager::getMode(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getDisplayMode();
    return -1;
}

int DeviceManager::setMode(int display, int mode)
{
    std::unique_lock<std::mutex> lock(mLock);

    bool hdmiNativeMode = false;
    DeviceControler *controler = getControlerById(display);

    if (!controler) {
        dd_error("Can not find controler for %d\n", display);
        return -1;
    }

    if (controler->type() == DISP_OUTPUT_TYPE_HDMI && mode == HDMI_AUTO_MODE) {
        mode = controler->getHDMINativeMode();
        hdmiNativeMode = true;
    }

    if (!controler->isSupportMode(mode)) {
        dd_error("device(type=%d) not support mode %d", controler->type(),
                 mode);
        return -1;
    }

    if (controler->type() == DISP_OUTPUT_TYPE_HDMI) {
        setHdmiUserSetting(hdmiNativeMode ? HDMI_AUTO_MODE : mode);
        controler->setHdmiUserSetting(hdmiNativeMode ? HDMI_AUTO_MODE : mode);
    }
    return controler->setDisplayMode(mode);
}

int DeviceManager::isSupportedMode(int display, int mode)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->isSupportMode(mode) ? 1 : 0;
    return 0;
}

int DeviceManager::getSupportedModes(int display, std::vector<int>& outlist)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getSupportModeList(outlist);
    return -1;
}

int DeviceManager::setAspectRatio(int display, int ratio)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->setAspectRatio(ratio);
    return -1;
}

int DeviceManager::getAspectRatio(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getAspectRatio();
    return -1;
}

int DeviceManager::setMargin(int display, int l, int r, int t, int b)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->setDisplayMargin(l, r, t, b);
    return -1;
}

int DeviceManager::getMargin(int display, std::vector<int>& out)
{
    int ret = -1;
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler) {
        int margin[4] = {0};
        ret = controler->getDisplayMargin(margin);
        out.push_back(margin[0]);
        out.push_back(margin[1]);
        out.push_back(margin[2]);
        out.push_back(margin[3]);
    }
    return ret;
}

int DeviceManager::isSupported3D(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->isSupport3D();
    return 0;
}

int DeviceManager::get3DLayerMode(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->get3DLayerMode();
    return 0;
}

int DeviceManager::set3DLayerMode(int display, int mode)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->set3DLayerMode(mode);
    return 0;
}

int DeviceManager::getSupportedPixelFormats(int display, std::vector<int>& formats)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getSupportPixelFormat(formats);
    return 0;
}

int DeviceManager::getPixelFormat(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getPixelFormat();
    return 0;
}

int DeviceManager::setPixelFormat(int display, int format)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->setPixelFormat(format);
    return 0;
}

int DeviceManager::getCurrentDataspace(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getCurrentDataspace();
    return 0;
}

int DeviceManager::getDataspaceMode(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->getDataspaceMode();
    return 0;
}

int DeviceManager::setDataspaceMode(int display, int mode)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler)
        return controler->setDataspaceMode(mode);
    return 0;
}

int DeviceManager::getEnhanceComponent(int display, int option)
{
    DeviceControler *controler = getControlerById(display);
    if (controler && controler->getEnhanceHandle()) {
        return controler->getEnhanceHandle()->getEnhanceComponent(option);
    }
    return 0;
}

int DeviceManager::setEnhanceComponent(int display, int option, int value)
{
    DeviceControler *controler = getControlerById(display);
    if (controler && controler->getEnhanceHandle()) {
        return controler->getEnhanceHandle()->setEnhanceComponent(option, value);
    }
    return 0;
}

int DeviceManager::onDataspaceChange(int dataspace)
{
    //dd_info("onDataspaceChange: request dataspace 0x%08x", dataspace);

    class DataspaceChangeMsg: public MessageBase {
        DeviceManager *manager;
        int dataspace;
    public:
        DataspaceChangeMsg(DeviceManager *m, int d)
            : manager(m), dataspace(d) {
        }

        virtual bool handler() {
            manager->handleDataspaceChange(dataspace);
            return true;
        }
    };

    /*
     * device->onDataspaceChange() will callback into hwc,
     * So, use async handle here to avoid death lock.
     */
    android::sp<MessageBase> msg = new DataspaceChangeMsg(this, dataspace);
    mMessageThread->postMessage(msg);
    return 0;
}

void DeviceManager::handleDataspaceChange(int dataspace)
{
    //dd_info("handleDataspaceChange: dataspace 0x%08x", dataspace);
    if (mDisplayDevices.count(0) == 1) {
        DeviceControler *device = mDisplayDevices[0];
        device->onDataspaceChange(dataspace);
    }
}

int DeviceManager::dump(std::string& out)
{
    android::String8 buf;
    std::unique_lock<std::mutex> lock(mLock);

    buf.appendFormat("Hotplug policy: %s\n", mPolicy->getDebugName().c_str());
    buf.appendFormat("Devices (%zd entries)\n", mDisplayDevices.size());
    for (const auto& device : mDisplayDevices) {
        buf.appendFormat(" + Device [%d] ==>\n", device.first);
        device.second->dump(buf);
    }
    out.append(buf.string());
    return 0;
}

int DeviceManager::configHdcp(bool enable)
{
    std::unique_lock<std::mutex> lock(mLock);
    if (mHdcpManager == nullptr) {
        mHdcpManager = std::make_unique<HdcpManager>();
    }
    return mHdcpManager->configHdcp(enable);
}

int DeviceManager::getConnectedHdcpLevel() const
{
    std::unique_lock<std::mutex> lock(mLock);
    if (mHdcpManager)
        return mHdcpManager->getConnectedHdcpLevel();
    return HdcpManager::HdcpManager::HDCP_UNKNOWN;
}

int DeviceManager::getAuthorizedStatus() const
{
    std::unique_lock<std::mutex> lock(mLock);
    if (mHdcpManager)
        return mHdcpManager->getAuthorizedStatus();
    return HdcpManager::ERROR;
}

/**
 * @name       :getHDMINativeMode
 * @brief      :get best resolution from edid
 * @param[IN]  :display:index of display device
 * @param[OUT] :NONE
 * @return     :- resolution mode id(vic)
 *              - -1 if native mode not found
 *              - -2 if current display is not hdmi
 */
int DeviceManager::getHDMINativeMode(int display)
{
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);
    if (controler && controler->type() == DISP_OUTPUT_TYPE_HDMI)
        return controler->getHDMINativeMode();
    else
        return -2;
}

void DeviceManager::setHdmiUserSetting(int strategy)
{
    char property[PROPERTY_VALUE_MAX] = {0};
    sprintf(property, "%d", strategy);
    property_set(HDMI_USER_STRATEGY, property);
}

/**
 * @name       :getHdmiUserSetting
 * @brief      :get best resolution from edid
 * @param[IN]  :display:index of display device
 * @param[OUT] :NONE
 * @return     :- 255:hdmi auto mode
 *		- dispmode defined in sunxi_display2.h
 *		- -2:if not hdmi device
 */
int DeviceManager::getHdmiUserSetting(int display)
{
    char property[PROPERTY_VALUE_MAX] = {0};
    std::unique_lock<std::mutex> lock(mLock);
    DeviceControler *controler = getControlerById(display);

    if (controler && controler->type() == DISP_OUTPUT_TYPE_HDMI) {
        if (0 >= property_get(HDMI_USER_STRATEGY, property, NULL)) {
            dd_error("property_get '%s' failed", HDMI_USER_STRATEGY);
            return HDMI_AUTO_MODE;
        }
        int strategy = atoi(property);
        dd_info("hdmi user strategy: %d", strategy);
        return strategy;
    } else
        return -2;
}
