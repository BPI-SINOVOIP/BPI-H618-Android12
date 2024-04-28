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

#ifndef __DEVICE_CONTROLER_H__
#define __DEVICE_CONTROLER_H__

#include <vector>
#include <utils/String8.h>
#include "persist_attr.h"
#include "device/hardware_ctrl.h"
#include "hardware/sunxi_display2.h"
#include "IHWCPrivateService.h"
#include "displayd/device_manager.h"

template <typename T>
class LatchState {
public:
    LatchState(T initialValue)
      : mPendingValue(initialValue),
        mValue(initialValue) {}

    T getValue() const { return mValue; }
    T getPendingValue() const { return mPendingValue; }
    void setPending(T value) { mPendingValue = value; }

    bool isDirty() const { return mPendingValue != mValue; }
    void latch() { mValue = mPendingValue; }

private:
    T mPendingValue;
    T mValue;
};

class DeviceControler : public HardwareCtrl {
public:
    DeviceControler(
            int displayEngineIndex,
            int type,
            sunxi::IHWCPrivateService& client);

    virtual ~DeviceControler();

    void dump(android::String8& str);
    int type() const {return mType;};
    int setLogicalId(int id);
    int setConnectState(int connected);
    void updateDisplayAttribute(struct disp_device_config *config);
    void present();

    int setDisplayMode(int mode);
    int setDisplayMargin(int l, int r, int t, int b);
    int setAspectRatio(int ratio);
    int getHdmiConnectState(int type);
    int getDisplayMode();
    int getAspectRatio();
    int getPixelFormat();
    int getDataspaceMode();
    int getCurrentDataspace();
    int getDisplayMargin(int *margin);

    virtual int performOptimalConfig(int enable) = 0;
    virtual int performDefaultConfig(int enable) = 0;
    virtual int setDisplayModeImpl(int mode);
    virtual int setPixelFormat(int format);
    virtual int setDataspaceMode(int mode);

    // Respon for layer's dataspace change
    virtual int onDataspaceChange(int dataspace);

    virtual int getSupportModeList(std::vector<int>& out) = 0;
    virtual int getSupportPixelFormat(std::vector<int>& out) = 0;
    virtual int getSupportDataspace(std::vector<int>& out) = 0;

    virtual bool isSupportMode(int mode) = 0;
    virtual bool isSupport3D() { return false; };

    virtual bool isCurrent3DOutput();
    virtual int get3DLayerMode();
    virtual int set3DLayerMode(int mode);
    virtual void onDeviceReconnected();
    virtual int getHDMINativeMode() { return -1; };
    virtual int getHdmiUserSetting() { return 0; };
    virtual int setHdmiUserSetting(int mode) { return 0;};
private:
    virtual void dumpHook(android::String8& str) = 0;

protected:
    void hwcBlank(int blank);
    void hwcSetDataspace(int dataspace);

    // mClient call into hwc impl
    sunxi::IHWCPrivateService& mClient;
    const int mType;
    const char *mDeviceName;
    int mLogicalId;
    LatchState<int> mConnectState;

    /* Attrbute need to be Saved */
    PersistAttr<disp_device_config> mDeviceConfig;
    PersistAttr<Margin> mMargin;
    PersistAttr<dataspace_t> mDataspace;
    PersistAttr<pixelformat_t> mPixelformat;
    PersistAttr<aspect_ratio_t> mAspectRatio;

    int m3dLayerMode;
};

#endif
