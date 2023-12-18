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

#ifndef _TABLET_DISPLAY_CONFIG_H_
#define _TABLET_DISPLAY_CONFIG_H_

#include "DisplayConfigImpl.h"
#include "enhance/EnhanceBase.h"

namespace sunxi {

class TabletDisplayConfig: public DisplayConfigImpl {
public:
    TabletDisplayConfig(IHWCPrivateService& client)
        : mHWComposer(client),
          mEnhanceHandle(createEnhanceHandle())
    { }

    int setDisplayArgs(int display, int cmd1, int cmd2, int data) override {
        switch (cmd1) {
            case kEnhanceSetting:
                return mEnhanceHandle->setEnhanceComponent(cmd2, data);
            case kSmartBackLight:
                return mEnhanceHandle->setSmartBackLight(data);
            case kTemperature:
                return mEnhanceHandle->setEnhanceComponent(
                        EnhanceBase::kColorTemperature, data);
            default:
                return mHWComposer.setDisplayArgs(display, cmd1, cmd2, data);
        }
    }

    int getEnhanceComponent(int display, int item) {
        ALOGV("TabletDisplayConfig getEnhanceComponent %d %d", display, item);
        return mEnhanceHandle->getEnhanceComponent(item);
    }

    int setEnhanceComponent(int display, int item, int value) {
        ALOGV("TabletDisplayConfig setEnhanceComponent %d %d %d", display, item, value);
        return mEnhanceHandle->setEnhanceComponent(item, value);
    }

    int dump(std::string& out) override {
        out += "\n TabletDisplayConfig \n";
        return 0;
    }

    int setEinkMode(int mode) override {
        return mHWComposer.setEinkMode(mode);
    }

    int setEinkBufferFd(int fd) override {
        return mHWComposer.setEinkBufferFd(fd);
    }

    int updateEinkRegion(int left, int top, int right, int bottom) override {
        return mHWComposer.updateEinkRegion(left, top, right, bottom);
    }

    int setEinkUpdateArea(int left, int top, int right, int bottom) override {
        return mHWComposer.setEinkUpdateArea(left, top, right, bottom);
    }

    int forceEinkRefresh(bool rightNow) override {
        return mHWComposer.forceEinkRefresh(rightNow);
    }

private:
    static const int kEnhanceSetting = 1;
    static const int kSmartBackLight = 2;
    static const int kTemperature    = 3;

    IHWCPrivateService& mHWComposer;
    std::unique_ptr<EnhanceBase> mEnhanceHandle;
};

} // namespace sunxi
#endif
