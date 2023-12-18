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

#include <sstream>
#include <android-base/stringprintf.h>
#include <sync/sync.h>

#include "Debug.h"
#include "Display.h"
#include "utils.h"
#include "private_handle.h"
#include "sunxi_eink.h"

using android::base::StringPrintf;

namespace sunxi {

Display::Display(hwc2_display_t id, std::shared_ptr<DeviceManager>& manager)
    : mHandle(id),
      mType(HWC2::DisplayType::Physical),
      mDeviceManager(manager),
      mClientTarget(std::make_shared<Layer>()),
      mColorMode(0),
      mActiveConfig(),
      mEventHandler(nullptr),
      mFrameNumber(0),
      mConnection()
{
    mConnection = mDeviceManager->createDeviceConnection(mHandle);
    mConnection.setDisplayEventCallback(this);
    mConnection->skipFrameEnable(!primaryDisplay());
    initDisplayConfig();
}

Display::~Display()
{
    DLOGI("Display[%lld] %p disconnect", mHandle, this);
    mConnection->setPowerMode(HWC2_POWER_MODE_OFF);
    mConnection = DeviceConnection();
    mDeviceManager->disconnect(mHandle);
}

HWC2::Error Display::acceptDisplayChanges()
{
    for (auto& l : mLayerStack) {
        l.second->acceptCompositionChanged();
    }
  return HWC2::Error::None;
}

HWC2::Error Display::createLayer(hwc2_layer_t *layer)
{
    std::shared_ptr<Layer> l = std::make_shared<Layer>();
    mLayerStack.emplace(l->id(), l);
    *layer = l->id();
    return HWC2::Error::None;
}

HWC2::Error Display::destroyLayer(hwc2_layer_t layer)
{
    if (mLayerStack.find(layer) == mLayerStack.end()) {
        DLOGE("Failed to destroy unknown layer (%llu)", layer);
        return HWC2::Error::BadLayer;
    }

    mLayerStack.erase(layer);
    return HWC2::Error::None;
}

HWC2::Error Display::getActiveConfig(hwc2_config_t *config)
{
    *config = mActiveConfig.configId;
    return HWC2::Error::None;
}

HWC2::Error Display::getChangedCompositionTypes(uint32_t *num_elements,
            hwc2_layer_t *layers,
            int32_t *types)
{
    uint32_t numberChanges= 0;
    for (auto& layer : mLayerStack){
        if (layer.second->compositionChanged()) {
            if (layers && numberChanges < *num_elements)
                layers[numberChanges] = layer.first;
            if (types && numberChanges < *num_elements)
                types[numberChanges] =
                    static_cast<int32_t>(layer.second->compositionFromValidated());
            numberChanges++;
        }
    }
    if (!layers && !types)
        *num_elements = numberChanges;

    return HWC2::Error::None;
}

HWC2::Error Display::getClientTargetSupport(uint32_t /* width */, uint32_t /* height */,
            int32_t /* format */, int32_t dataspace)
{
    if (dataspace != HAL_DATASPACE_UNKNOWN &&
            dataspace != HAL_DATASPACE_STANDARD_UNSPECIFIED)
        return HWC2::Error::Unsupported;
    return HWC2::Error::None;
}

HWC2::Error Display::getColorModes(uint32_t *num_modes, int32_t *modes)
{
    if (!modes)
        *num_modes = 1;
    else
        *modes = HAL_COLOR_MODE_NATIVE;

    return HWC2::Error::None;
}

HWC2::Error Display::getDisplayAttribute(
        hwc2_config_t id, int32_t attribute, int32_t *value)
{
    syncDisplayConfig();

    auto what = static_cast<HWC2::Attribute>(attribute);
    switch (what) {
      case HWC2::Attribute::Width:
        *value = mActiveConfig.width;
        break;
      case HWC2::Attribute::Height:
        *value = mActiveConfig.height;
        break;
      case HWC2::Attribute::VsyncPeriod:
        // in nanoseconds
        *value = 1000 * 1000 * 1000 / mActiveConfig.refreshRate;
        break;
      case HWC2::Attribute::DpiX:
        *value = mActiveConfig.dpix;
        break;
      case HWC2::Attribute::DpiY:
        *value = mActiveConfig.dpiy;
        break;
      default:
        *value = -1;
        return HWC2::Error::BadConfig;
    }
    return HWC2::Error::None;
}

HWC2::Error Display::getDisplayConfigs(uint32_t *num_configs, hwc2_config_t *configs)
{
    // TODO: add multi configs supported.
    if (!configs) {
        *num_configs = 1;
        return HWC2::Error::None;
    }

    configs[0] = mActiveConfig.configId;
    *num_configs = 1;
    return HWC2::Error::None;
}

HWC2::Error Display::getDisplayName(uint32_t *size, char *name)
{
    std::ostringstream stream;
    stream << "display-" << mHandle;
    std::string str = stream.str();
    size_t length = str.length();
    if (!name) {
        *size = length;
        return HWC2::Error::None;
    }

    *size = std::min<uint32_t>(static_cast<uint32_t>(length - 1), *size);
    strncpy(name, str.c_str(), *size);
    return HWC2::Error::None;
}

HWC2::Error Display::getDisplayRequests(int32_t *display_requests,
            uint32_t *num_elements, hwc2_layer_t *layers,
            int32_t *layer_requests)
{
    // TODO:
    return HWC2::Error::None;
}

HWC2::Error Display::getDisplayType(int32_t *type)
{
    *type = static_cast<int32_t>(mType);
    return HWC2::Error::None;
}

HWC2::Error Display::getDozeSupport(int32_t *support)
{
    *support = 0;
    return HWC2::Error::None;
}

HWC2::Error Display::getHdrCapabilities(uint32_t *num_types, int32_t * /* types */,
            float * /* max_luminance */,
            float * /* max_average_luminance */,
            float * /* min_luminance */) {
    *num_types = 0;
    return HWC2::Error::None;
}

HWC2::Error Display::getReleaseFences(uint32_t *num_elements, hwc2_layer_t *layers,
            int32_t *fences)
{
    uint32_t num_layers = 0;
    for (auto& layer : mLayerStack) {
        if (layers && fences) {
            if (num_layers < *num_elements) {
                layers[num_layers] = layer.first;
                fences[num_layers] = layer.second->takeReleaseFence();
            } else {
                DLOGW("%s: Overflow %d/%d", __func__, num_layers, *num_elements);
                return HWC2::Error::None;
            }
        }
        num_layers++;
    }
    *num_elements = num_layers;
    return HWC2::Error::None;
}

HWC2::Error Display::setActiveConfig(hwc2_config_t config)
{
    // TODO:
    return HWC2::Error::None;
}

HWC2::Error Display::setClientTarget(buffer_handle_t target, int32_t acquire_fence,
            int32_t dataspace, hwc_region_t /* damage */)
{
    uniquefd fence(acquire_fence);
    mClientTarget->setBufferHandle(target);
    mClientTarget->setAcquireFence(fence.dup());
    mClientTarget->setLayerDataspace(dataspace);
    return HWC2::Error::None;
}

void Display::setupClientTarget()
{
    mClientTarget->setLayerBlendMode(HWC2_BLEND_MODE_PREMULTIPLIED);
    mClientTarget->setLayerCompositionType(HWC2_COMPOSITION_DEVICE);
    mClientTarget->setLayerPlaneAlpha(1.0f);
    mClientTarget->setLayerDisplayFrame(hwc_rect_t{0, 0, mActiveConfig.width, mActiveConfig.height});
    mClientTarget->setLayerSourceCrop(
            hwc_frect_t{
            0.0f,
            0.0f,
            static_cast<float>(mActiveConfig.width),
            static_cast<float>(mActiveConfig.height)});
    mClientTarget->setLayerZOrder(0xFF);
}

HWC2::Error Display::setColorMode(int32_t mode)
{
    if (mode < HAL_COLOR_MODE_NATIVE || mode > HAL_COLOR_MODE_DISPLAY_P3)
        return HWC2::Error::BadParameter;
    if (mode != HAL_COLOR_MODE_NATIVE && mode != HAL_COLOR_MODE_SRGB)
        return HWC2::Error::Unsupported;

    mColorMode = mode;
    return HWC2::Error::None;
}

HWC2::Error Display::setColorTransform(const float *matrix, int32_t hint)
{
    mConnection->setColorTransform(matrix, hint);
    return HWC2::Error::None;
}

HWC2::Error Display::setOutputBuffer(buffer_handle_t buffer, int32_t release_fence)
{
    // TODO: Need virtual display support
    DLOGW("Unsupported function: %s", __func__);
    return HWC2::Error::Unsupported;
}

HWC2::Error Display::setPowerMode(int32_t mode)
{
    if (mode < HWC2_POWER_MODE_OFF || mode > HWC2_POWER_MODE_DOZE_SUSPEND)
        return HWC2::Error::BadParameter;

    int error = mConnection->setPowerMode(mode);
    return static_cast<HWC2::Error>(error);
}

HWC2::Error Display::setVsyncEnabled(int32_t enabled)
{
    if (enabled && !primaryDisplay()) {
        DLOGW("Enable vsync on non primary display!");
        return HWC2::Error::Unsupported;
    }
    mConnection->setVsyncEnabled(enabled == HWC2_VSYNC_ENABLE);
    return HWC2::Error::None;
}

HWC2::Error Display::validateDisplay(uint32_t *num_types, uint32_t *num_requests)
{
    DTRACE_FUNC();

    *num_types = 0;
    *num_requests = 0;
    mContext.reset(createComposition());
    for (auto& layer : mContext->InputLayers) {
        if (layer->compositionChanged())
            ++*num_types;
    }
    return *num_types ? HWC2::Error::HasChanges : HWC2::Error::None;
}

HWC2::Error Display::getRenderIntents(int32_t mode, uint32_t *outNumIntents,
        int32_t *outIntents)
{
    if (mode < HAL_COLOR_MODE_NATIVE || mode > HAL_COLOR_MODE_BT2100_HLG)
        return HWC2::Error::BadParameter;
    if (mode != HAL_COLOR_MODE_NATIVE)
        return HWC2::Error::Unsupported;

    if (!outIntents) {
        *outNumIntents = 1;
    } else {
        *outIntents = HAL_RENDER_INTENT_COLORIMETRIC;
    }
    return HWC2::Error::None;
}

HWC2::Error Display::setColorModeWithRenderIntent(int32_t mode, int32_t intent)
{
    if (mode < HAL_COLOR_MODE_NATIVE || mode > HAL_COLOR_MODE_BT2100_HLG)
        return HWC2::Error::BadParameter;
    if (intent < HAL_RENDER_INTENT_COLORIMETRIC || intent > HAL_RENDER_INTENT_TONE_MAP_ENHANCE)
        return HWC2::Error::BadParameter;
    if (mode != HAL_COLOR_MODE_NATIVE || intent != HAL_RENDER_INTENT_COLORIMETRIC)
        return HWC2::Error::Unsupported;
    return HWC2::Error::None;
}

HWC2::Error Display::presentDisplay(int32_t *retire_fence)
{
    DTRACE_FUNC();

    *retire_fence = -1;
    if (!mContext) {
        DLOGW("presentDisplay: not validateDisplay yet");
        return HWC2::Error::BadParameter;
    }

    commitFrame();
    dumpInputBuffer();

    // The retire fence returned here is for the last frame,
    // So return it and promote the next retire fence
    *retire_fence = mRetireFence.release();
    mRetireFence = std::move(mContext->RetireFence);

    ++mFrameNumber;
    return HWC2::Error::None;
}

CompositionContext* Display::createComposition()
{
    DTRACE_FUNC();

    CompositionContext* ctx = new CompositionContext();
    if (ctx == nullptr) {
        DLOGE("Failed to allocate CompositionContext!");
        return nullptr;
    }

    // Insert visible layers and sort it by zorder
    for (auto& item : mLayerStack) {
		ctx->InputLayers.insert(item.second);
	}


    setupClientTarget();
    ctx->FramebufferTarget = mClientTarget;
    ctx->Viewport = mViewport;

    bool deviceChanged = mConnection.populateDevice();
    if (deviceChanged) {
        // when external display 's vsync frequency is slow then primary display,
        // the queued frame will block on commit thread,
        // so that we should enable skip frame feature for external display.
        mConnection->skipFrameEnable(!primaryDisplay());
    }

    // setup layer composition type
    mConnection->prepare(ctx);

    return ctx;
}

void Display::commitFrame()
{
    DTRACE_FUNC();

    // commit to hardware
    mConnection->commit(mContext.get());
}

void Display::dumpInputBuffer()
{
    if (!primaryDisplay()) {
        // only support dump primary display's input buffer
        return;
    }

    int zorder = 0;
    for (auto& layer : mContext->InputLayers) {
        if (DumpBufferRequest::get().requestDump(DumpBufferRequest::eInputBuffer, 1 << zorder)) {
            private_handle_t* handle = (private_handle_t *)layer->bufferHandle();
            if (handle == nullptr)
                return;

            char path[128] = {0};
            sprintf(path, "/data/dump_%s_%dx%d_%d.dat",
                    getHalPixelFormatString(handle->format),
                    handle->width,
                    handle->height,
                    mFrameNumber);
            sync_wait(layer->acquireFence(), 1000);
            dumpBuffer(layer->bufferHandle(), path);
            DLOGD("dump input buffer: %s", path);
        }
        zorder++;
    }
}

void Display::setEventHandler(EventHandler* handler)
{
    if (handler == nullptr) {
        DLOGE("Rejected attempt to clear handler");
        return;
    }
    mEventHandler = handler;
}

void Display::onVsync(int64_t timestamp)
{
    if (mEventHandler == nullptr) {
        DLOGW("vsyncHook: mEventHandler is nullptr");
        return;
    }
    mEventHandler->onVSyncReceived(mHandle, timestamp);
}

void Display::requestRefresh()
{
    if (mEventHandler == nullptr) {
        DLOGW("refreshHook: mEventHandler is nullptr");
        return;
    }
    mEventHandler->onRefreshReceived(mHandle);
}

void Display::initDisplayConfig()
{
    const DeviceBase::Config& defaultConfig = mConnection->getDefaultConfig();
    mActiveConfig = defaultConfig;
    mViewport = Rect(mActiveConfig.width, mActiveConfig.height);
}

void Display::syncDisplayConfig()
{
    // TODO: add multi configs supported
    // We just sync the refreshRate here
    mActiveConfig.refreshRate = mConnection->getRefreshRate();
}

void Display::dump(std::string& out)
{
    out += StringPrintf(" Display[%" PRIu64 "] %dx%d DPI=%d",
            mHandle, mActiveConfig.width, mActiveConfig.height, mActiveConfig.dpix);
    out += StringPrintf(" FrameNumber=%d", mFrameNumber);
    out += StringPrintf(" LayerCount=%zu\n", mLayerStack.size());
    std::string devinfo;
    mConnection->dump(devinfo);
    out += StringPrintf("   DeviceInfo:\n") + devinfo;
}

} // namespace sunxi
