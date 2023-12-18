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

#include "WriteBackManager.h"
#include "WriteBackDef.h"
#include "Display.h"
#include "NormalWriteBackDisp2.h"
#include "SelfWriteBackDisp2.h"
#include "Debug.h"
//#include "helper.h"

using namespace sunxi;

std::shared_ptr<WriteBackManager> WriteBackManager::mInstance;

std::shared_ptr<WriteBackManager> WriteBackManager::getWbInstance()
{
    if (mInstance == nullptr) {
        mInstance = std::make_shared<WriteBackManager>();
    }
    return mInstance;
}

WriteBackMode WriteBackManager::getWriteBackMode()
{
    WriteBackMode mode = NO_WB;
#ifdef WRITE_BACK_MODE
    if (WRITE_BACK_MODE == NO_WB) {
        mode = NO_WB;
    } else if (WRITE_BACK_MODE == ON_NEED) {
        mode = ON_NEED;
    } else if (WRITE_BACK_MODE == ALWAYS) {
        mode = ALWAYS;
    } else if (WRITE_BACK_MODE == SELF_WB) {
        mode = SELF_WB;
    }
#endif
    return mode;
}

WriteBackManager::WriteBackManager()
    :mSrcIdx(SRC),mDstIdx(DST)
{
    mbHotplug = false;
    mMode = getWriteBackMode();
    if (mMode == NO_WB) {
        mWriteBack = nullptr;
    } else if (mMode == ON_NEED) {
        mWriteBack = std::make_shared<NormalWriteBackDisp2>(mSrcIdx);
    } else if (mMode == ALWAYS) {
        mWriteBack = std::make_shared<NormalWriteBackDisp2>(mSrcIdx);
    } else if (mMode == SELF_WB) {
        mWriteBack = std::make_shared<SelfWriteBackDisp2>(mSrcIdx);
    }

    mSrcBlank = true;
    mDstBlank = true;
    mSrcState = 0;
    mDstState = 0;
}

void WriteBackManager::setHotplugState(bool pluging)
{
    mbHotplug = pluging;
    DLOGD("mbHotplug = %d", mbHotplug);
}

bool WriteBackManager::isSupportWb()
{
    if (mMode == NO_WB) {
        return false;
    } else {
        return true;
    }
}

void WriteBackManager::onMarginChanged(int id, int hpersent, int vpersent)
{
    if (id != mSrcIdx || mWriteBack == nullptr) {
        return;
    }
    mWriteBack->setMargin(hpersent, vpersent);
}

void WriteBackManager::onScreenSizeChanged(int id, int width, int height)
{
    if (id != mSrcIdx || mWriteBack == nullptr) {
        return;
    }
    mWriteBack->setScreenSize(width, height);
}

void WriteBackManager::onFrameSizeChanged(int id, int width, int height)
{
    if (id != mSrcIdx || mWriteBack == nullptr) {
        return;
    }
    mWriteBack->setFrameSize(width, height);
}

void WriteBackManager::onPowerModeChanged(int id, bool isBlank)
{
    if (id == mSrcIdx) {
        mSrcBlank = isBlank;
    } else if (id == mDstIdx) {
        mDstBlank = isBlank;
    }
}

int WriteBackManager::onFrameCommit(int hwid, unsigned int syncnum, disp_layer_config2* conf,
        int lyrNum)
{
    std::shared_ptr<Display> srcDpy, dstDpy, fakeDpy;
    if (hwid != mSrcIdx || mWriteBack == nullptr || mMode != SELF_WB) {
        return -1;
    }
    for (auto&it : mDisplays) {
        int32_t logicalId = it.second->getDisplayId();
        int hwId = mDevicePool->getHwIdx(logicalId);
        if (hwId == mSrcIdx && logicalId != WBID) {
            srcDpy = it.second;
        } else if (hwId == mDstIdx) {
            dstDpy = it.second;
        }
        if (logicalId == WBID && hwId == mSrcIdx) {
            fakeDpy = it.second;
        }
    }

    if (fakeDpy == nullptr || fakeDpy.get() == nullptr) {
        DLOGW("self write back need fakeDpy, but=%p", fakeDpy.get());
        return -1;
    }

    return mWriteBack->performFrameCommit(hwid, syncnum, conf, lyrNum);
}

int32_t WriteBackManager::getWriteBackId()
{
    return mSrcIdx;
}

int32_t WriteBackManager::setDisplays(std::map<hwc2_display_t, std::shared_ptr<Display>>& dpys)
{
    mDisplays.clear();
    for (auto& item : dpys) {
        mDisplays.emplace(item.first, item.second);
        DLOGD("display =%p", item.second.get());
    }

    DLOGD("display size=%d", mDisplays.size());
    mbHotplug = false;
    return 0;
}

std::shared_ptr<Display> WriteBackManager::handleValidate(std::shared_ptr<Display> dpy)
{
    bool isWb = false;
    std::unordered_map<hwc2_layer_t, std::shared_ptr<Layer>>* lyrStk = nullptr;
    std::shared_ptr<Display> srcDpy, dstDpy, fakeDpy;

    if (dpy == nullptr || mDstBlank || mSrcBlank) {
        return dpy;
    }
    if (mbHotplug) {
        dpy->setValidMode(ValidMode::NORMAL);
        return dpy;
    }
    lyrStk = dpy->getLayerStack();
    for (auto& item : (*lyrStk)) {
        if (isNeedWb(item.second))
            isWb = true;
    }

    if (!isWb) {
        return dpy;
    }

    for (auto&it : mDisplays) {
        int32_t logicalId = it.second->getDisplayId();
        int hwId = mDevicePool->getHwIdx(logicalId);
        if (hwId == mSrcIdx && logicalId != WBID) {
            srcDpy = it.second;
        } else if (hwId == mDstIdx) {
            dstDpy = it.second;
        }
        if (logicalId == WBID && hwId == mSrcIdx) {
            fakeDpy = it.second;
        }
    }

    if (srcDpy == nullptr && fakeDpy == nullptr) {
        DLOGW("srcdy=%p, fakeDpy=%p", srcDpy.get(), fakeDpy.get());
    }

    if (dstDpy == nullptr) {
        /*no dst, so no wb*/
        return dpy;
    } else if (srcDpy == nullptr && dpy == dstDpy && fakeDpy != nullptr) {
        if (mDstState != 0) {
            DLOGW("not accept present. mDstState =%d", mDstState);
            return dpy;
        }
        mDstState = 1;
        /*no src, so fake one to validate*/
        fakeDpy->cpInputInfo(dstDpy);
        dpy->setValidMode(ValidMode::BYPASS);
        return fakeDpy;
    } else if (dpy == srcDpy) {
        if (mSrcState != 0) {
            DLOGW("not accept present. mSrcState =%d", mSrcState);
            return dpy;
        }
        mSrcState = 1;
        /*src and dst both on, just validate src*/
        return dpy;
    } else if (dpy == dstDpy) {
        if (mDstState != 0) {
            DLOGW("not accept present. mDstState =%d", mDstState);
            return dpy;
        }
        mDstState = 1;
        /*skip dst's validate which should do with wb buf*/
        dpy->setValidMode(ValidMode::BYPASS);
        return dpy;
    } else {
        DLOGW("dpy is not wanted!!!");
        return dpy;
    }
    return dpy;
}

std::shared_ptr<Display> WriteBackManager::handlePresent(std::shared_ptr<Display> dpy)
{
    bool isWb = false;
    std::unordered_map<hwc2_layer_t, std::shared_ptr<Layer>>* lyrStk = nullptr;
    std::shared_ptr<Display> srcDpy, dstDpy, fakeDpy;

    if (dpy == nullptr || mDstBlank || mSrcBlank) {
        return dpy;
    }
    if (mbHotplug) {
        dpy->setValidMode(ValidMode::NORMAL);
        return dpy;
    }
    lyrStk = dpy->getLayerStack();
    for (auto& item : (*lyrStk)) {
        if (isNeedWb(item.second))
            isWb = true;
    }

    if (!isWb && WRITE_BACK_MODE == ON_NEED) {
        std::shared_ptr<Layer> wbBuf = nullptr;
        wbBuf = mWriteBack->acquireLayer();
        if (wbBuf != nullptr && wbBuf.get() != nullptr) {
            mWriteBack->releaseLayer(wbBuf, -1);
        }
        return dpy;
    }

    for (auto&it : mDisplays) {
        int32_t logicalId = it.second->getDisplayId();
        int hwId = mDevicePool->getHwIdx(logicalId);
        if (hwId == mSrcIdx && logicalId != WBID) {
            srcDpy = it.second;
            continue;
        } else if (hwId == mDstIdx) {
            dstDpy = it.second;
            continue;
        }
        if (logicalId == WBID) {
            fakeDpy = it.second;
        }
    }

    if (dstDpy == nullptr) {
        return dpy;
    } else if (srcDpy == nullptr && fakeDpy != nullptr && dpy == dstDpy) {
        uint32_t n, m;
        int fence;
        std::shared_ptr<Layer> wbBuf = nullptr;
        if (mDstState != 1) {
            DLOGW("not accept validate. mDstState =%d", mDstState);
            return dpy;
        }
        mDstState = 0;
        /*wb one frame of fakeDpy*/
        n = dup(fakeDpy->getRetireFence());
        if (mWriteBack->writebackOneFrame(n)) {
            DLOGE("writeback failed");
            return dpy;
        }
        /*dst use one wb buf to validate and present*/
        wbBuf = mWriteBack->acquireLayer();
        if (wbBuf != nullptr && wbBuf.get() != nullptr) {
            dpy->setWriteBackLayer(wbBuf);
            dpy->setValidMode(ValidMode::WRITEBACK);
            dpy->validateDisplay(&n, &m);
            dpy->setValidMode(ValidMode::NORMAL);
            dpy->setFenceUseless();
            dpy->presentDisplay(&fence);
            mWriteBack->releaseLayer(wbBuf, fence);
            return fakeDpy;
        }
        dpy->skipNextPresent();
        dpy->setValidMode(NORMAL);
        return dpy;
    } else if (dpy == srcDpy) {
        /*has src, just present and wb one frame*/
        int fence;
        if (mSrcState != 1) {
            DLOGW("not accept present. mSrcState =%d", mSrcState);
            return dpy;
        }
        mSrcState = 0;
        fence = dup(dpy->getRetireFence());
        if (mWriteBack->writebackOneFrame(fence)) {
            DLOGE("writeback failed");
        }
        return dpy;
    } else if (dpy == dstDpy) {
        uint32_t n, m;
        std::shared_ptr<Layer> wbBuf = nullptr;

        if (mDstState != 1) {
            DLOGW("not accept validate. mDstState =%d", mDstState);
            return dpy;
        }
        mDstState = 0;
        /*dst use one wb buf to validate and present*/
        wbBuf = mWriteBack->acquireLayer();
        if (wbBuf == nullptr || wbBuf.get() == nullptr) {
            dpy->skipNextPresent();
            DLOGW("acquire failed");
            return dpy;
        }
        dpy->setWriteBackLayer(wbBuf);
        dpy->setValidMode(ValidMode::WRITEBACK);
        dpy->validateDisplay(&n, &m);
        mWriteBack->releaseLayer(wbBuf, -1);
        dpy->setValidMode(ValidMode::NORMAL);
        dpy->setFenceUseless();
        return dpy;
    } else {
        DLOGW("dpy is not wanted!!!");
        return dpy;
    }

    return dpy;
}

int32_t WriteBackManager::setDevicePool(std::shared_ptr<IDeviceFactory>& dpool)
{
    mDevicePool = dpool;
    return 0;
}

extern bool isAfbcBuffer(buffer_handle_t buf);

bool WriteBackManager::isNeedWb(std::shared_ptr<Layer>& layer) {
    if (mMode == NO_WB || layer == NULL) {
        return false;
    }

    if (mMode == ALWAYS || mMode == SELF_WB) {
        return true;
    } else if (mMode == ON_NEED) {
        hwc_rect_t sourceCrop = layer->sourceCrop();
        if (isAfbcBuffer(layer->bufferHandle())) {
            return true;
        }
        if (sourceCrop.right - sourceCrop.left >= 3840
            && sourceCrop.bottom - sourceCrop.top >= 2048) {
            return true;
        }
    }
    return false;
}

WriteBackManager::~WriteBackManager()
{
    mWriteBack.reset();
    mDevicePool.reset();
    mDisplays.clear();
}
