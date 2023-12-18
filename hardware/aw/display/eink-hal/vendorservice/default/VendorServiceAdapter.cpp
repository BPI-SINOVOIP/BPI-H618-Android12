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
#include "VendorServiceAdapter.h"

namespace sunxi {

/* static */
VendorServiceAdapter* VendorServiceAdapter::getService() {
    static VendorServiceAdapter sInstance;
    return &sInstance;
}

VendorServiceAdapter::VendorServiceAdapter()
    : mInterface(nullptr),
      mDisplayConfigService(nullptr),
      mEventCallback(nullptr)
{
    initialize();
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
    DLOGI("register IVendorInterface: %p", intf);
}

int VendorServiceAdapter::registerCallback(IHWCPrivateService::EventCallback* cb)
{
    mEventCallback = cb;
    return 0;
}

void VendorServiceAdapter::setDebugTag(int32_t tag)
{
    DLOGI("tag=%08x", tag);
    Debug::get().setDebugTag(tag, true);
}

int VendorServiceAdapter::setDisplayArgs(int display, int cmd1, int cmd2, int data)
{
    switch (cmd1) {
        case 0x0000DEAD:
            setDebugTag(data);
            break;
        case 0x0010DEAD:
            DumpBufferRequest::get().updateRequest(display, cmd2, data);
            break;
        case 0x0020DEAD:
            notifyVendorDebugRequest(display, cmd2, data);
            break;
        default:
            DLOGW("unknow request cmd=%08x", cmd1);
            return -1;
    }
    return 0;
}

int VendorServiceAdapter::setEinkMode(int mode)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
	return mInterface->setEinkMode(mode);
}

int VendorServiceAdapter::setEinkBufferFd(int fd)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
	return mInterface->setEinkBufferFd(fd);
}

int VendorServiceAdapter::updateEinkRegion(int left, int top, int right, int bottom)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
	return mInterface->updateEinkRegion(left, top, right, bottom);
}

int VendorServiceAdapter::setEinkUpdateArea(int left, int top, int right, int bottom)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
	return mInterface->setEinkUpdateArea(left, top, right, bottom);
}

int VendorServiceAdapter::forceEinkRefresh(bool rightNow)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
    return mInterface->forceEinkRefresh(rightNow);
}

#define FREEZE_OUTPUT (0)

void VendorServiceAdapter::notifyVendorDebugRequest(int display, int tag, int data)
{
    std::lock_guard<std::mutex> lock(mInterfaceMutex);
    switch (tag) {
        case FREEZE_OUTPUT:
            mInterface->freezeOuput(display, data == 1 ? true : false);
            break;
        default:
            DLOGW("unknow vendor debug request");
            break;
    }
}

/* unused api */
#pragma clang diagnostic push "-Wunused-parameter"
int VendorServiceAdapter::blank(int display, int enable) { return 0; }
int VendorServiceAdapter::switchDevice(const DeviceTable& tables) { return 0; }
int VendorServiceAdapter::setOutputMode(int display, int type, int mode) { return 0; }
int VendorServiceAdapter::setMargin(int display, int l, int r, int t, int b) { return 0; }
int VendorServiceAdapter::setVideoRatio(int display, int ratio) { return 0; }
int VendorServiceAdapter::set3DMode(int display, int mode) { return 0; }
int VendorServiceAdapter::setDataspace(int display, int dataspace) { return 0; }
SNRApi* VendorServiceAdapter::getSNRInterface() { return nullptr; }
#pragma clang diagnostic pop

} // namespace sunxi
