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

#include <utils/Log.h>
#include "DisplayConfigImpl.h"

namespace sunxi {

#define UNUSED(x) (void)x

#undef  LOG_TAG
#define LOG_TAG "DisplayConfig"

#define DEBUG(x, ...) \
    ALOGD("%s: " x, __PRETTY_FUNCTION__, ##__VA_ARGS__)

int DisplayConfigImpl::getPortType(int display)
{
    UNUSED(display);
    return 0 /* DISP_OUTPUT_TYPE_NONE */;
}

int DisplayConfigImpl::setPortType(int display, int type)
{
    UNUSED(display);
    UNUSED(type);
    return -1;
}

int DisplayConfigImpl::getMode(int display)
{
    UNUSED(display);
    return -1;
}

int DisplayConfigImpl::setMode(int display, int mode)
{
    UNUSED(display);
    UNUSED(mode);
    return -1;
}

int DisplayConfigImpl::getSupportedModes(int display, std::vector<int>& outlist)
{
    UNUSED(display);
    UNUSED(outlist);
    return -1;
}

int DisplayConfigImpl::supported3D(int display)
{
    UNUSED(display);
    return 0 /* Not supported */;
}

int DisplayConfigImpl::get3DLayerMode(int display)
{
    UNUSED(display);
    return 0 /* 2D original */;
}

int DisplayConfigImpl::set3DLayerMode(int display, int mode)
{
    UNUSED(display);
    UNUSED(mode);
    return -1;
}

int DisplayConfigImpl::getSupportedPixelFormats(int display, std::vector<int>& formats)
{
    UNUSED(display);
    UNUSED(formats);
    return -1;
}

int DisplayConfigImpl::getPixelFormat(int display)
{
    UNUSED(display);
    return 0 /* default pixel format */;
}

int DisplayConfigImpl::setPixelFormat(int display, int format)
{
    UNUSED(display);
    UNUSED(format);
    return -1;
}

int DisplayConfigImpl::getCurrentDataspace(int display)
{
    UNUSED(display);
    return 0;
}

int DisplayConfigImpl::getDataspaceMode(int display)
{
    UNUSED(display);
    return 0;
}

int DisplayConfigImpl::setDataspaceMode(int display, int mode)
{
    UNUSED(display);
    UNUSED(mode);
    return -1;
}

int DisplayConfigImpl::getAspectRatio(int display)
{
    UNUSED(display);
    return 0;
}

int DisplayConfigImpl::setAspectRatio(int display, int ratio)
{
    UNUSED(display);
    UNUSED(ratio);
    return 0;
}

int DisplayConfigImpl::getMargin(int display, std::vector<int>& margin)
{
    UNUSED(display);
    UNUSED(margin);
    return 0;
}

int DisplayConfigImpl::setMargin(int display, int l, int r, int t, int b)
{
    UNUSED(display);
    UNUSED(l);
    UNUSED(r);
    UNUSED(t);
    UNUSED(b);
    return 0;
}

int DisplayConfigImpl::getEnhanceComponent(int display, int item)
{
    UNUSED(display);
    UNUSED(item);
    return 0;
}

int DisplayConfigImpl::setEnhanceComponent(int display, int item, int value)
{
    UNUSED(display);
    UNUSED(item);
    UNUSED(value);
    return 0;
}

bool DisplayConfigImpl::supportedSNRSetting(int display)
{
    UNUSED(display);
    return false;
}

int DisplayConfigImpl::getSNRInfo(int display, ::vendor::display::config::V1_0::SNRInfo& info)
{
    UNUSED(display);
    UNUSED(info);
    return 0;
}

int DisplayConfigImpl::setSNRInfo(int display, const ::vendor::display::config::V1_0::SNRInfo& info)
{
    UNUSED(display);
    UNUSED(info);
    return 0;
}

int DisplayConfigImpl::configHdcp(bool enable)
{
    UNUSED(enable);
    return -1;
}

int DisplayConfigImpl::getConnectedHdcpLevel() const
{
    return 0;
}

int DisplayConfigImpl::getAuthorizedStatus() const
{
    return 0;
}

int DisplayConfigImpl::getHDMINativeMode(int display)
{
    UNUSED(display);
    return 0;
}

int DisplayConfigImpl::getHdmiUserSetting(int display)
{
    UNUSED(display);
    return 0;
}

int DisplayConfigImpl::setEinkMode(int mode)
{
    UNUSED(mode);
    return 0;
}

int DisplayConfigImpl::setEinkBufferFd(int fd)
{
    UNUSED(fd);
    return 0;
}

int DisplayConfigImpl::updateEinkRegion(int left, int top, int right, int bottom)
{
    UNUSED(left);
    UNUSED(top);
    UNUSED(right);
    UNUSED(bottom);
    return 0;
}

int DisplayConfigImpl::setEinkUpdateArea(int left, int top, int right, int bottom)
{
    UNUSED(left);
    UNUSED(top);
    UNUSED(right);
    UNUSED(bottom);
    return 0;
}

int DisplayConfigImpl::forceEinkRefresh(bool rightNow)
{
    UNUSED(rightNow);
    return 0;
}
} // namespace sunxi

