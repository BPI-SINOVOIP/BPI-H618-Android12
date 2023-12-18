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

#ifndef _HOMLET_DISPLAY_CONFIG_H_
#define _HOMLET_DISPLAY_CONFIG_H_

#include <memory>
#include "DisplayConfigImpl.h"
#include "displayd/device_manager.h"
#include "snr/SNRManager.h"

namespace sunxi {

class HomletDisplayConfig: public DisplayConfigImpl {
public:
    HomletDisplayConfig(IHWCPrivateService& client)
        : mHWComposer(client),
          mDeviceManager(std::make_unique<DeviceManager>(client)),
          mSNRManager(new SNRManager(client))
    { }
   ~HomletDisplayConfig() { }

    // Base display output setting for tablet platform.
    int setDisplayArgs(int display, int cmd1, int cmd2, int data) {
        return mHWComposer.setDisplayArgs(display, cmd1, cmd2, data);
    }

    // Interface for homlet platform.
    int getPortType(int display) {
        return mDeviceManager->getType(display);
    }
    // int setPortType(int display, int type);

    int getMode(int display) {
        return mDeviceManager->getMode(display);
    }
    int setMode(int display, int mode) {
        return mDeviceManager->setMode(display, mode);
    }
    int getSupportedModes(int display, std::vector<int>& outlist) {
        return mDeviceManager->getSupportedModes(display, outlist);
    }

    int supported3D(int display) {
        return mDeviceManager->isSupported3D(display);
    }
    int get3DLayerMode(int display) {
        return mDeviceManager->get3DLayerMode(display);
    }
    int set3DLayerMode(int display, int mode) {
        return mDeviceManager->set3DLayerMode(display, mode);
    }

    int getSupportedPixelFormats(int display, std::vector<int>& formats) {
        return mDeviceManager->getSupportedPixelFormats(display, formats);
    }
    int getPixelFormat(int display) {
        return mDeviceManager->getPixelFormat(display);
    }
    int setPixelFormat(int display, int format) {
        return mDeviceManager->setPixelFormat(display, format);
    }

    int getCurrentDataspace(int display) {
        return mDeviceManager->getCurrentDataspace(display);
    }
    int getDataspaceMode(int display) {
        return mDeviceManager->getDataspaceMode(display);
    }
    int setDataspaceMode(int display, int mode) {
        return mDeviceManager->setDataspaceMode(display, mode);
    }

    int getAspectRatio(int display) {
        return mDeviceManager->getAspectRatio(display);
    }
    int setAspectRatio(int display, int ratio) {
        return mDeviceManager->setAspectRatio(display, ratio);
    }
    int getMargin(int display, std::vector<int>& margin) {
        return mDeviceManager->getMargin(display, margin);
    }
    int setMargin(int display, int l, int r, int t, int b) {
        return mDeviceManager->setMargin(display, l, r, t, b);
    }
    int getEnhanceComponent(int display, int item) {
        return mDeviceManager->getEnhanceComponent(display, item);
    }
    int setEnhanceComponent(int display, int item, int value) {
        return mDeviceManager->setEnhanceComponent(display, item, value);
    }

    bool supportedSNRSetting(int display) override {
        return mSNRManager->platformSupportSNR(display);
    }
    int getSNRInfo(int display,
            ::vendor::display::config::V1_0::SNRInfo& info) override {
        if (mSNRManager != nullptr) {
            return mSNRManager->getSNRInfo(display, info);
        }
        return -ENODEV;
    }
    int setSNRInfo(int display,
            const ::vendor::display::config::V1_0::SNRInfo& info) override {
        if (mSNRManager != nullptr) {
            return mSNRManager->setSNRInfo(display, info);
        }
        return -ENODEV;
    }

    int configHdcp(bool enable) override {
        return mDeviceManager->configHdcp(enable);
    }
    int getConnectedHdcpLevel() const override {
        return mDeviceManager->getConnectedHdcpLevel();
    }
    int getAuthorizedStatus() const override {
        return mDeviceManager->getAuthorizedStatus();
    }

    int dump(std::string& out) {
        return mDeviceManager->dump(out);
    }

    int getHDMINativeMode(int display) {
        return mDeviceManager->getHDMINativeMode(display);
    }

    int getHdmiUserSetting(int display) {
        return mDeviceManager->getHdmiUserSetting(display);
    }

private:
    IHWCPrivateService& mHWComposer;
    std::unique_ptr<DeviceManager> mDeviceManager;
    std::unique_ptr<SNRManager> mSNRManager;
};

} // namespace sunxi
#endif
