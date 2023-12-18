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

#ifndef __IHWC_PRIVATE_SERVICE_H__
#define __IHWC_PRIVATE_SERVICE_H__

#include <utils/RefBase.h>
#include "DeviceTable.h"

namespace sunxi {

class SNRApi;

class IHWCPrivateService
{
public:
    // Handle legacy display commands from framework.
    virtual int setDisplayArgs(int display, int cmd1, int cmd2, int data) = 0;

    // Sync with device/device_hdmi.h.
    enum _dataspace_mode {
        eDataspaceSdr  = 0x1001,
        eDataspaceHdr  = 0x1002,
        eDataspaceWcg  = 0x1003,
        eDataspaceHdrp = 0x1004,
        eDataspaceDv   = 0x1005,
    };

    class EventCallback {
    public:
        virtual ~EventCallback() { };
        virtual void onDataspaceChanged(int dataspace) = 0;
    };

    // Device apis for homlet platform.
    virtual int switchDevice(const DeviceTable& tables) = 0;
    virtual int blank(int display, int enable) = 0;
    virtual int setOutputMode(int display, int type, int mode) = 0;

    virtual int setMargin(int display, int l, int r, int t, int b) = 0;
    virtual int setVideoRatio(int display, int ratio) = 0;
    virtual int set3DMode(int display, int mode) = 0;
    virtual int setDataspace(int display, int dataspace) = 0;
    virtual int registerCallback(EventCallback* cb) = 0;
    virtual int updateEinkRegion(int left, int top, int right, int bottom) { return (left + top + right + bottom); };
    virtual int setEinkBufferFd(int fd) { return fd; };
    virtual int setEinkMode(int mode) { return mode; };
    virtual int forceEinkRefresh(bool rightNow) { return rightNow; };
    virtual int setEinkUpdateArea(int left, int top, int right, int bottom) { return (left + top + right + bottom); };

    virtual void setDebugTag(int tag) = 0;
    virtual SNRApi* getSNRInterface() = 0;

    virtual ~IHWCPrivateService() { }
};

} // namespace sunxi
#endif
