/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HWC_LAYER_H_
#define ANDROID_HWC_LAYER_H_

#include <memory>
#include <atomic>
#include <vector>
#include <hardware/hwcomposer2.h>

#include "uniquefd.h"

namespace sunxi {

class Layer {
public:
    Layer();
   ~Layer();

    // Layer hooks
    HWC2::Error setCursorPosition(int32_t x, int32_t y);
    HWC2::Error setLayerBlendMode(int32_t mode);
    HWC2::Error setLayerBuffer(buffer_handle_t buffer, int32_t acquire_fence);
    HWC2::Error setLayerColor(hwc_color_t color);
    HWC2::Error setLayerCompositionType(int32_t type);
    HWC2::Error setLayerDataspace(int32_t dataspace);
    HWC2::Error setLayerDisplayFrame(hwc_rect_t frame);
    HWC2::Error setLayerPlaneAlpha(float alpha);
    HWC2::Error setLayerSidebandStream(const native_handle_t *stream);
    HWC2::Error setLayerSourceCrop(hwc_frect_t crop);
    HWC2::Error setLayerSurfaceDamage(hwc_region_t damage);
    HWC2::Error setLayerTransform(int32_t transform);
    HWC2::Error setLayerVisibleRegion(hwc_region_t visible);
    HWC2::Error setLayerZOrder(uint32_t z);

    HWC2::Composition compositionFromSurfaceFlinger() const {
        return mRequestType;
    }
    HWC2::Composition compositionFromValidated() const {
        return mValidatedType;
    }
    void acceptCompositionChanged() {
        mRequestType = mValidatedType;
    }
    bool compositionChanged() const {
        return mRequestType != mValidatedType;
    }
    void setValidatedType(HWC2::Composition type) {
        mValidatedType = type;
    }
    void setBufferHandle(buffer_handle_t handle) {
        mBuffer = handle;
    }
    int acquireFence() {
        return mAcquireFence.get();
    }
    int takeAcquireFence() {
        return mAcquireFence.release();
    }
    void setAcquireFence(int fd) {
        uniquefd fence(fd);
        mAcquireFence = std::move(fence);
    }
    int releaseFence() {
        return mReleaseFence[1].get();
    }
    int takeReleaseFence() {
        return mReleaseFence[1].release();
    }
    void setReleaseFence(int fd) {
        uniquefd fence(fd);
        mReleaseFence[1] = std::move(mReleaseFence[0]);
        mReleaseFence[0] = std::move(fence);
    }

    inline hwc2_layer_t        id()           const { return mId;           }
    inline buffer_handle_t     bufferHandle() const { return mBuffer;       }
    inline uint32_t            zOrder()       const { return mZOrder;       }
    inline float               alpha()        const { return mAlpha;        }
    inline HWC2::BlendMode     blending()     const { return mBlending;     }
    inline HWC2::Transform     transform()    const { return mTransform;    }
    inline android_dataspace_t dataspace()    const { return mDataspace;    }
    inline hwc_rect_t          displayFrame() const { return mDisplayFrame; }
    inline hwc_rect_t          sourceCrop()   const { return mSourceCrop;   }
    inline hwc_color_t         solidColor()   const { return mSolidColor;   }

    class DestroyedListener {
    public:
        virtual void onLayerDestroyed(hwc2_layer_t id) = 0;
        virtual ~DestroyedListener() = default;
    };
    void registerLayerDestroyListener(std::weak_ptr<DestroyedListener> listener);

private:
    static std::atomic<hwc2_layer_t> mNextLayerId;

    // Unique id to identify each layer
    hwc2_layer_t mId;

    // mRequestType:   stores the initial type given to us by surfaceflinger
    // mValidatedType: stores the type after validateDisplay
    HWC2::Composition mRequestType;
    HWC2::Composition mValidatedType;

    // Layer attributes
    uint32_t mZOrder;
    float mAlpha;
    HWC2::BlendMode mBlending;
    buffer_handle_t mBuffer;
    hwc_rect_t mDisplayFrame;
    hwc_rect_t mSourceCrop;
    HWC2::Transform mTransform;
    int32_t mCursorX, mCursorY;
    android_dataspace_t mDataspace;
    hwc_color_t mSolidColor;

    uniquefd mAcquireFence;

    // the meaning of the release fence is:
    // the content of the buffer for frame N has now replaced the previous content.
    // so use a defer fence to achieve this goal.
    uniquefd mReleaseFence[2];

    // callback on layer destroy
    std::vector<std::weak_ptr<DestroyedListener>> mDestroyedListener;
};

// helper functions
bool isBlendingLayer(const std::shared_ptr<Layer>& layer);
bool isViChannelExclusiveRgbFormat(const std::shared_ptr<Layer>& layer);
bool isYuvFormat(const std::shared_ptr<Layer>& layer);
bool isSkipLayer(const std::shared_ptr<Layer>& layer);
bool isSoildLayer(const std::shared_ptr<Layer>& layer);
bool isPremultLayer(const std::shared_ptr<Layer>& layer);

int getLayerPixelFormat(const std::shared_ptr<Layer>& layer);
size_t computeLayerMemoryFootprint(const std::shared_ptr<Layer>& layer);

} // namespace sunxi

#endif

