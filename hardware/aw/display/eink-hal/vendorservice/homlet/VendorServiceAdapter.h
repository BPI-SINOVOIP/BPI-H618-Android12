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

#ifndef VENDOR_SERVICE_ADAPTER_H
#define VENDOR_SERVICE_ADAPTER_H

#include <map>
#include <mutex>
#include <memory>

#include "DisplayConfigService.h"
#include "IHWCPrivateService.h"
#include "IVendorInterface.h"
#include "snr.h"

namespace sunxi {

class VendorServiceAdapter: public IHWCPrivateService {
public:
    static VendorServiceAdapter* getService();
    void setVendorInterface(IVendorInterface* intf);

private:
    /* API define in IHWCPrivateService.h */
    int setDisplayArgs(int display, int cmd1, int cmd2, int data) override;
    int blank(int display, int enable) override;
    int switchDevice(const DeviceTable& tables) override;
    int setOutputMode(int display, int type, int mode) override;
    int setMargin(int display, int l, int r, int t, int b) override;
    int setVideoRatio(int display, int ratio) override;
    int set3DMode(int display, int mode) override;
    int setDataspace(int display, int dataspace) override;
    int registerCallback(IHWCPrivateService::EventCallback* cb) override;
    void setDebugTag(int32_t tag) override;
    SNRApi* getSNRInterface() override;

    int updateConnectedDevicesInternal();

private:
    void initialize();
    VendorServiceAdapter();
   ~VendorServiceAdapter() = default;

private:
    std::mutex mInterfaceMutex;
    IVendorInterface* mInterface;
    std::map<int, std::unique_ptr<IVendorInterface::DeviceInfo>> mDeviceList;

    android::sp<sunxi::DisplayConfigService> mDisplayConfigService;
    IHWCPrivateService::EventCallback *mEventCallback;
};

} // namespace sunxi
#endif
