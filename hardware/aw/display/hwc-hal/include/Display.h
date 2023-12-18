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

#ifndef ANDROID_HWC_DISPLAY_H_
#define ANDROID_HWC_DISPLAY_H_

#include <set>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <hardware/hwcomposer2.h>

#include "DeviceManager.h"
#include "Layer.h"
#include "Rect.h"
#include "Debug.h"

namespace sunxi {

typedef enum {
    NORMAL = 0,
    BYPASS,
    WRITEBACK,
} ValidMode;


enum class Hdr : int32_t {
    /**
     * Device supports Dolby Vision HDR  */
    DOLBY_VISION = 1,
    /**
     * Device supports HDR10  */
    HDR10 = 2,
    /**
     * Device supports hybrid log-gamma HDR  */
    HLG = 3,
};

class Display;

class SortLayersByZ {
public:
    bool operator()(const std::shared_ptr<Layer>& lhs,
            const std::shared_ptr<Layer>& rhs) const {
        return lhs->zOrder() < rhs->zOrder();
    }
};

struct CompositionContext {
    std::set<std::shared_ptr<Layer>, SortLayersByZ> InputLayers;
    std::shared_ptr<Layer> FramebufferTarget;
    Rect Viewport;
    uniquefd RetireFence;
};

class Display : public DisplayEventCallback {
public:
    Display(hwc2_display_t id, std::shared_ptr<DeviceManager>& manager);
   ~Display();

    // HWC Hooks
    HWC2::Error acceptDisplayChanges();
    HWC2::Error createLayer(hwc2_layer_t *layer);
    HWC2::Error destroyLayer(hwc2_layer_t layer);
    HWC2::Error getActiveConfig(hwc2_config_t *config);
    HWC2::Error getChangedCompositionTypes(uint32_t *num_elements,
            hwc2_layer_t *layers, int32_t *types);
    HWC2::Error getClientTargetSupport(uint32_t width, uint32_t height,
            int32_t format, int32_t dataspace);
    HWC2::Error getColorModes(uint32_t *num_modes, int32_t *modes);
    HWC2::Error getDisplayAttribute(hwc2_config_t config, int32_t attribute, int32_t *value);
    HWC2::Error getDisplayConfigs(uint32_t *num_configs, hwc2_config_t *configs);
    HWC2::Error getDisplayName(uint32_t *size, char *name);
    HWC2::Error getDisplayRequests(int32_t *display_requests,
            uint32_t *num_elements, hwc2_layer_t *layers, int32_t *layer_requests);
    HWC2::Error getDisplayType(int32_t *type);
    HWC2::Error getDozeSupport(int32_t *support);
    HWC2::Error getHdrCapabilities(uint32_t *num_types, int32_t *types,
            float *max_luminance,
            float *max_average_luminance,
            float *min_luminance);
    HWC2::Error getReleaseFences(uint32_t *num_elements, hwc2_layer_t *layers, int32_t *fences);
    HWC2::Error presentDisplay(int32_t *retire_fence);
    HWC2::Error setActiveConfig(hwc2_config_t config);
    HWC2::Error setClientTarget(buffer_handle_t target,
            int32_t acquire_fence,
            int32_t dataspace,
            hwc_region_t damage);
    HWC2::Error setColorMode(int32_t mode);
    HWC2::Error setColorTransform(const float *matrix, int32_t hint);
    HWC2::Error setOutputBuffer(buffer_handle_t buffer, int32_t release_fence);
    HWC2::Error setPowerMode(int32_t mode);
    HWC2::Error setVsyncEnabled(int32_t enabled);
    HWC2::Error validateDisplay(uint32_t *num_types, uint32_t *num_requests);
    HWC2::Error getRenderIntents(int32_t mode, uint32_t *outNumIntents, int32_t *outIntents);
    HWC2::Error setColorModeWithRenderIntent(int32_t mode, int32_t intent);

    std::shared_ptr<Layer> getLayer(hwc2_layer_t layer) {
        if (mLayerStack.count(layer))
            return mLayerStack.at(layer);
        return nullptr;
    }

    void onVsync(int64_t timestamp) override;
    void requestRefresh() override;

    class EventHandler {
    public:
        virtual void onVSyncReceived(hwc2_display_t disp, int64_t timestamp) = 0;
        virtual void onRefreshReceived(hwc2_display_t disp) = 0;
        virtual ~EventHandler() = default;
    };

    void setEventHandler(EventHandler* handler);
    void dump(std::string& out);

private:
    bool primaryDisplay() const {
        return mHandle == HWC_DISPLAY_PRIMARY;
    }

    /* Sync display config from hardware */
    void initDisplayConfig();
    void syncDisplayConfig();

    void setupClientTarget();
    CompositionContext *createComposition();
    void commitFrame();
    void dumpInputBuffer();

private:
    hwc2_display_t mHandle;
    HWC2::DisplayType mType;
    std::shared_ptr<DeviceManager> mDeviceManager;
    std::unordered_map<hwc2_layer_t, std::shared_ptr<Layer>> mLayerStack;
    std::shared_ptr<Layer> mClientTarget;
    uniquefd mRetireFence;

    int32_t mColorMode;
    DeviceBase::Config mActiveConfig;
    Rect mViewport;

    std::shared_ptr<CompositionContext> mContext;

    EventHandler* mEventHandler;
    uint32_t mFrameNumber;

    // Hardware display device
    DeviceConnection mConnection;

    /*for writeback*/
    ValidMode mValidMode;
    bool mSkipPresent;
    bool mFenceUseless;

    std::shared_ptr<Layer> mWriteBackLyr;

public:
    std::unordered_map<hwc2_layer_t, std::shared_ptr<Layer>>* getLayerStack() {
        return &mLayerStack;
    }

    std::shared_ptr<Layer>& getClientTarget() {
        return mClientTarget;
    }

    void cpInputInfo(std::shared_ptr<Display>& dpy) {
        std::unordered_map<hwc2_layer_t, std::shared_ptr<Layer>>* lyrStk = nullptr;

        if (dpy == nullptr) {
            DLOGW("nullptr dpy");
            return;
        }
        lyrStk = dpy->getLayerStack();
        mLayerStack.clear();
        for (auto& item : (*lyrStk)) {
            mLayerStack.insert(item);
        }
        if (mLayerStack.size() <= 0) {
            DLOGW("copy layers failed, size=%d", mLayerStack.size());
            return;
        }
        if (mClientTarget != dpy->getClientTarget()) {
            mClientTarget = dpy->getClientTarget();
        }
    }

    hwc2_display_t getDisplayId() {
        return mHandle;
    }

    void setValidMode(ValidMode mode) {
        mValidMode = mode;
    }

    void skipNextPresent() {
        mSkipPresent = true;
    }

    void setFenceUseless() {
        mFenceUseless = true;
    }

    int32_t getRetireFence() {
        return mRetireFence.get();
    }

    void setWriteBackLayer(std::shared_ptr<Layer> lyr) {
        mWriteBackLyr = lyr;
    }
    /*for writeback end*/

};

} // namespace sunxi

#endif

