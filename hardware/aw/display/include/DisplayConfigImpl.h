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

#ifndef _DISPLAY_CONFIG_IMPL_H__
#define _DISPLAY_CONFIG_IMPL_H__

#include <string>
#include <vector>

#include <utils/RefBase.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>

namespace sunxi {

class DisplayConfigImpl: public android::RefBase {
public:
    virtual ~DisplayConfigImpl() { }

    static DisplayConfigImpl* getInstance();

    // Base display output setting for tablet platform.
    virtual int setDisplayArgs(int display,
                               int cmd1,
                               int cmd2,
                               int data) = 0;

    // Interface for homlet platform.
    virtual int getPortType(int display);
    virtual int setPortType(int display, int type);

    virtual int getMode(int display);
    virtual int setMode(int display, int mode);
    virtual int getSupportedModes(int display, std::vector<int>& outlist);

    virtual int supported3D(int display);
    virtual int get3DLayerMode(int display);
    virtual int set3DLayerMode(int display, int mode);

    virtual int getSupportedPixelFormats(int display, std::vector<int>& formats);
    virtual int getPixelFormat(int display);
    virtual int setPixelFormat(int display, int format);

    virtual int getCurrentDataspace(int display);
    virtual int getDataspaceMode(int display);
    virtual int setDataspaceMode(int display, int mode);

    virtual int getAspectRatio(int display);
    virtual int setAspectRatio(int display, int ratio);
    virtual int getMargin(int display, std::vector<int>& margin);
    virtual int setMargin(int display, int l, int r, int t, int b);
    virtual int getEnhanceComponent(int display, int item);
    virtual int setEnhanceComponent(int display, int item, int value);

    virtual bool supportedSNRSetting(int display);
    virtual int getSNRInfo(int display, ::vendor::display::config::V1_0::SNRInfo& info);
    virtual int setSNRInfo(int display, const ::vendor::display::config::V1_0::SNRInfo& info);

    virtual int configHdcp(bool enable);
    virtual int getConnectedHdcpLevel() const;
    virtual int getAuthorizedStatus() const;

    virtual int dump(std::string& out) = 0;
    virtual int getHDMINativeMode(int display);
    virtual int getHdmiUserSetting(int display);

    virtual int setEinkMode(int mode);
    virtual int setEinkBufferFd(int fd);
    virtual int updateEinkRegion(int left, int top, int right, int bottom);
    virtual int forceEinkRefresh(bool rightNow);
    virtual int setEinkUpdateArea(int left, int top, int right, int bottom);
};

} // namespace sunxi
#endif
