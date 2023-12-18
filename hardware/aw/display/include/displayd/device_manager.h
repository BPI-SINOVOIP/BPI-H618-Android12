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

#ifndef __DEVICE_MANAGER_H__
#define __DEVICE_MANAGER_H__

#include <vector>
#include <unordered_map>
#include <mutex>

#include <utils/StrongPointer.h>

namespace sunxi {
    class IHWCPrivateService;
    class DeviceConfig;
}

class HotplugMonitor;
class MessageQueueThread;
class OutputPolicy;
class DeviceControler;
class HdcpManager;

#define HDMI_AUTO_MODE (255)
#define HDMI_USER_STRATEGY  "persist.sys.hdmistrategy"

class DeviceManager {
public:
    DeviceManager(sunxi::IHWCPrivateService& client);
    virtual ~DeviceManager();

    void initialize();
    void hotplug(int type, int connected);
    void waitHwcAdapterSetup();

    /*
     * Interface for IDisplayConfig client.
     * Protect by std::mutex mLock
     */
    int getType(int display);
    int getMode(int display);
    int setMode(int display, int mode);
    int isSupportedMode(int display, int mode);
    int getSupportedModes(int display, std::vector<int>& outlist);

    int setAspectRatio(int display, int ratio);
    int getAspectRatio(int display);
    int setMargin(int display, int l, int r, int t, int b);
    int getMargin(int display, std::vector<int>& out);

    int isSupported3D(int display);
    int get3DLayerMode(int display);
    int set3DLayerMode(int display, int mode);

    int getSupportedPixelFormats(int display, std::vector<int>& formats);
    int getPixelFormat(int display);
    int setPixelFormat(int display, int format);

    int getCurrentDataspace(int display);
    int getDataspaceMode(int display);
    int setDataspaceMode(int display, int mode);

    int getEnhanceComponent(int display, int option);
    int setEnhanceComponent(int display, int option, int value);

    int configHdcp(bool enable);
    int getConnectedHdcpLevel() const;
    int getAuthorizedStatus() const;

    /*
     * Dump debug info for DisplayDebug tool
     */
    int dump(std::string& out);

    /*
     * Callback from HWC when dataspace change
     */
    int onDataspaceChange(int dataspace);

    /**
     * @name       :getHDMINativeMode
     * @brief      :get best resolution from edid
     * @param[IN]  :display:index of display device
     * @param[OUT] :NONE
     * @return     :- resolution mode id(vic)
     *              - -1 if native mode not found
     *              - -2 if current display is not hdmi
     */
    int getHDMINativeMode(int display);

    /**
     * @name       :getHdmiUserSetting
     * @brief      :get best resolution from edid
     * @param[IN]  :display:index of display device
     * @param[OUT] :NONE
     * @return     :- 255:hdmi auto mode
     *              - dispmode defined in sunxi_display2.h
     *              - -2:if not hdmi device
     */
    int getHdmiUserSetting(int display);


private:
    inline DeviceControler *getControler(int type);
    inline DeviceControler *getControlerById(int id);
    void setupDefaultOutput();

    void setHdmiUserSetting(int strategy);

    friend class MessageQueueThread;
    void handleDataspaceChange(int dataspace);
    void threadedStartup();

    class HotplugCallback;
    const HotplugCallback *mHotplugCallback;
    HotplugMonitor *mHotplugMonitor;

    class HWCCallback;
    HWCCallback* mHwcCallback;
    android::sp<MessageQueueThread> mMessageThread;

    std::unique_ptr<OutputPolicy> mPolicy;
    std::vector<DeviceControler*> mDevices;
    std::unordered_map<int, DeviceControler*> mDisplayDevices;

    // mClient call into hwc impl
    sunxi::IHWCPrivateService& mClient;
    mutable std::mutex mLock;

    std::unique_ptr<HdcpManager> mHdcpManager;
    std::unique_ptr<sunxi::DeviceConfig> mDeviceConfig;
};

#endif
