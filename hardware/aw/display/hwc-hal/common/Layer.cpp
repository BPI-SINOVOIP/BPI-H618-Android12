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

#include <cmath>
#include <hardware/graphics-sunxi.h>

#include "Debug.h"
#include "Layer.h"
#include "private_handle.h"

namespace sunxi {

std::atomic<hwc2_layer_t> Layer::mNextLayerId(1);

Layer::Layer()
    : mId(mNextLayerId++),
      mRequestType(HWC2::Composition::Invalid),
      mValidatedType(HWC2::Composition::Invalid),
      mZOrder(0),
      mAlpha(1.0f),
      mBlending(HWC2::BlendMode::None),
      mBuffer(nullptr),
      mTransform(HWC2::Transform::None),
      mCursorX(0),
      mCursorY(0),
      mDataspace(HAL_DATASPACE_UNKNOWN),
      mDestroyedListener() { }

Layer::~Layer()
{
    for (auto& item : mDestroyedListener) {
        std::shared_ptr<DestroyedListener> listener = item.lock();
        listener->onLayerDestroyed(mId);
    }
}

HWC2::Error Layer::setCursorPosition(int32_t x, int32_t y)
{
    mCursorX = x;
    mCursorY = y;
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerBlendMode(int32_t mode) {
    mBlending = static_cast<HWC2::BlendMode>(mode);
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerBuffer(buffer_handle_t buffer, int32_t acquire_fence)
{
    uniquefd fence(acquire_fence);
    setBufferHandle(buffer);
    setAcquireFence(fence.dup());
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerColor(hwc_color_t color)
{
    mSolidColor = color;
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerCompositionType(int32_t type)
{
    mRequestType = static_cast<HWC2::Composition>(type);
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerDataspace(int32_t dataspace)
{
    mDataspace = static_cast<android_dataspace_t>(dataspace);
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerDisplayFrame(hwc_rect_t frame)
{
    mDisplayFrame = frame;
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerPlaneAlpha(float alpha)
{
    mAlpha = alpha;
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerSidebandStream(const native_handle_t *stream)
{
    DLOGW("Unsupported function: %s", __func__);
    return HWC2::Error::Unsupported;
}

HWC2::Error Layer::setLayerSourceCrop(hwc_frect_t crop)
{
    mSourceCrop.left   = std::ceil(crop.left);
    mSourceCrop.top    = std::ceil(crop.top);
    mSourceCrop.right  = std::ceil(crop.right);
    mSourceCrop.bottom = std::ceil(crop.bottom);
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerSurfaceDamage(hwc_region_t damage)
{
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerTransform(int32_t transform)
{
    mTransform = static_cast<HWC2::Transform>(transform);
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerVisibleRegion(hwc_region_t visible)
{
    return HWC2::Error::None;
}

HWC2::Error Layer::setLayerZOrder(uint32_t z)
{
    mZOrder = z;
    return HWC2::Error::None;
}


void Layer::registerLayerDestroyListener(std::weak_ptr<DestroyedListener> listener)
{
    mDestroyedListener.push_back(listener);
}

//------------------------------------------------------------------------------------//
// helper functions

bool isBlendingLayer(const std::shared_ptr<Layer>& layer)
{
    // TODO
    return (layer->blending() == HWC2::BlendMode::Premultiplied ||
        layer->blending() == HWC2::BlendMode::Coverage);
}

bool isViChannelExclusiveRgbFormat(const std::shared_ptr<Layer>& layer)
{
    if (layer->bufferHandle() == 0)
        return false;

    const private_handle_t *handle =
        reinterpret_cast<const private_handle_t *>(layer->bufferHandle());
    switch (handle->format) {
    case HAL_PIXEL_FORMAT_RGBA_1010102:
        return true;
    default:
        return false;
    }
}

bool isYuvFormat(const std::shared_ptr<Layer>& layer)
{
    if (layer->bufferHandle() == 0)
        return false;

    const private_handle_t *handle =
        reinterpret_cast<const private_handle_t *>(layer->bufferHandle());
    switch (handle->format) {
    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_AW_NV12:
    case HAL_PIXEL_FORMAT_AW_YV12_10bit:
    case HAL_PIXEL_FORMAT_AW_I420_10bit:
    case HAL_PIXEL_FORMAT_AW_NV21_10bit:
    case HAL_PIXEL_FORMAT_AW_NV12_10bit:
    case HAL_PIXEL_FORMAT_AW_P010_UV:
    case HAL_PIXEL_FORMAT_AW_P010_VU:
        return true;
    default:
        return false;
    }
}

bool isSkipLayer(const std::shared_ptr<Layer>& layer)
{
    if (layer->compositionFromSurfaceFlinger() == HWC2::Composition::Client)
        return true;
    return false;
}

bool isSoildLayer(const std::shared_ptr<Layer>& layer)
{
    return (layer->compositionFromSurfaceFlinger() == HWC2::Composition::SolidColor);
}

bool isPremultLayer(const std::shared_ptr<Layer>& layer)
{
    return (layer->blending() == HWC2::BlendMode::Premultiplied);
}

int getLayerPixelFormat(const std::shared_ptr<Layer>& layer)
{
    if (isSoildLayer(layer))
        return HAL_PIXEL_FORMAT_RGBA_8888;

    if (layer->bufferHandle()) {
        const private_handle_t *handle =
            reinterpret_cast<const private_handle_t *>(layer->bufferHandle());
        return handle ? handle->format : -1;
    }
    return -1;
}

size_t computeLayerMemoryFootprint(const std::shared_ptr<Layer>& layer)
{
    if (!layer || isSoildLayer(layer))
        return 0;

    float bytePerPixel = 4;
    int format = getLayerPixelFormat(layer);
    switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            bytePerPixel = 4;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            bytePerPixel = 3;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            bytePerPixel = 2;
            break;
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_AW_NV12:
        case HAL_PIXEL_FORMAT_AW_NV12_10bit:
        case HAL_PIXEL_FORMAT_AW_NV21_10bit:
        case HAL_PIXEL_FORMAT_AW_I420_10bit:
        case HAL_PIXEL_FORMAT_AW_YV12_10bit:
            bytePerPixel = 1.5f;
            break;
        case HAL_PIXEL_FORMAT_AW_P010_UV:
        case HAL_PIXEL_FORMAT_AW_P010_VU:
            bytePerPixel = 3;
            break;
        default:
            DLOGW("Unknown pixelformat(%d)", format);
            bytePerPixel = 4;
            break;
    }

    hwc_rect_t src = layer->sourceCrop();
    int w = src.right - src.left;
    int h = src.bottom - src.top;

    float footprint = w * h * bytePerPixel;

    // For scale down layers,
    // the bandwidth required by the layer needs to be increased !!!
    hwc_rect_t dispFrame = layer->displayFrame();
    w = dispFrame.right - dispFrame.left;
    h = dispFrame.bottom - dispFrame.top;
    const float factor = 1.5f;
    float dis_footprint = w * h * bytePerPixel;
    if (dis_footprint < footprint) {
        footprint += ((footprint - dis_footprint) * factor);
    }

    return ceil(footprint);
}

} // namespace sunxi

