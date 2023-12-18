/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef SUNXI_HWC_WRITEBACK_MANAGER_H_
#define SUNXI_HWC_WRITEBACK_MANAGER_H_

#include <map>
#include <unordered_map>
#include "WriteBackDef.h"
#include "Display.h"
#include "hardware/sunxi_display2.h"
#ifdef WRITE_BACK_MODE
#include "ImageMapper.h"
#endif

namespace sunxi {

    typedef enum {
        NO_WB,
        ON_NEED,
        ALWAYS,
        SELF_WB,
    } WriteBackMode;

    class WriteBackBase {
    public:
        virtual ~WriteBackBase() = default;
        WriteBackBase() = default;

        virtual int writebackOneFrame(int fence) = 0;
        virtual std::shared_ptr<Layer> acquireLayer() = 0;
        virtual int releaseLayer(std::shared_ptr<Layer>& lyr, int fence) = 0;
        virtual void setScreenSize(int width, int height) = 0;
        virtual void setFrameSize(int width, int height) = 0;
        virtual void setMargin(int hpersent, int vpersent) = 0;
        virtual int performFrameCommit(int hwid, unsigned int syncnum,
                disp_layer_config2* conf, int lyrNum) = 0;
    };

    class WriteBackManager {
    public:
        ~WriteBackManager();
        WriteBackManager();
        static std::shared_ptr<WriteBackManager> getWbInstance();
        std::shared_ptr<Display> getDisplay(hwc2_display_t id);
        int32_t getWriteBackId();
        int32_t setDisplays(std::map<hwc2_display_t, std::shared_ptr<Display>>& displays);
        std::shared_ptr<Display> handleValidate(std::shared_ptr<Display> dpy);
        std::shared_ptr<Display> handlePresent(std::shared_ptr<Display> dpy);
        int32_t setDevicePool(std::shared_ptr<IDeviceFactory>& dpool);
        bool isSupportWb();
        void onScreenSizeChanged(int id, int width, int height);
        void onFrameSizeChanged(int id, int width, int height);
        void onMarginChanged(int id, int hpersent, int vpersent);
        void onPowerModeChanged(int id, bool isBlank);
        int onFrameCommit(int hwid, unsigned int syncnum, disp_layer_config2* conf, int lyrNum);
        bool isNeedWb(std::shared_ptr<Layer>& layer);
        void setHotplugState(bool pluging);

    private:
        WriteBackMode getWriteBackMode();
        WriteBackMode mMode;
        int mSrcIdx;
        int mDstIdx;
        int mSrcState;/*0 accept validate; 1 accept present*/
        int mDstState;
        bool mSrcBlank;
        bool mDstBlank;
        std::map<hwc2_display_t, std::shared_ptr<Display>> mDisplays;
        std::shared_ptr<IDeviceFactory> mDevicePool;
        std::shared_ptr<WriteBackBase> mWriteBack;
        static std::shared_ptr<WriteBackManager> mInstance;
        bool mbHotplug;
    };

} // namespace sunxi

#endif // SUNXI_HWC_WRITEBACK_MANAGER_H_

