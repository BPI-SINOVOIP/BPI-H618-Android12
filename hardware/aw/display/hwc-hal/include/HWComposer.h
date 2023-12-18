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

#include <map>
#include <utility>
#include <hardware/hardware.h>
#include <hardware/hwcomposer2.h>
#include <WriteBackManager.h>

namespace sunxi {

class DeviceManager;
class Display;
class IDeviceFactory;
class DevicePowerManagerBase;

class HWComposer : public hwc2_device_t, Display::EventHandler {
public:
    static int open(const struct hw_module_t *module,
                    const char *name, struct hw_device_t **dev);
    HWComposer();
   ~HWComposer();

    // Event handler callback into graphic composer
    void onHotplugReceived(int32_t handle, bool connected);
    void onVSyncReceived(hwc2_display_t id, int64_t timestamp) override;
    void onRefreshReceived(hwc2_display_t id) override;

private:
    // hwc2_device_t hooks
    static int closeHook(hw_device_t *dev);
    static void getCapabilitiesHook(
            hwc2_device_t *dev, uint32_t *count, int32_t *capabilities);
    static hwc2_function_pointer_t getFunctionHook(
            struct hwc2_device *device, int32_t descriptor);

    HWC2::Error createVirtualDisplay(uint32_t width, uint32_t height,
            int32_t *format, hwc2_display_t *display);
    HWC2::Error destroyVirtualDisplay(hwc2_display_t display);
    uint32_t getMaxVirtualDisplayCount();
    HWC2::Error registerCallback(int32_t descriptor,
            hwc2_callback_data_t data, hwc2_function_pointer_t function);

    int initializeDevices();
    void terminateDevices();
    std::shared_ptr<Display> getDisplay(hwc2_display_t id);
    void waitForHotplugIssued(std::unique_lock<std::mutex>& lock);
    HWC2::Error setPowerMode(hwc2_display_t id, int32_t mode);
    void dump(uint32_t *size, char *buffer);

    template <typename PFN, typename T>
    static hwc2_function_pointer_t asFunctionPointer(T function) {
        static_assert(std::is_same<PFN, T>::value, "Incompatible function pointer");
        return reinterpret_cast<hwc2_function_pointer_t>(function);
    }

    template <typename T, typename HookType, HookType func, typename... Args>
    static T DeviceHook(hwc2_device_t *dev, Args... args) {
        HWComposer* hwc = static_cast<HWComposer*>(dev);
        return static_cast<T>(((*hwc).*func)(std::forward<Args>(args)...));
    }

    template <typename HookType, HookType func, typename... Args>
    static int32_t DisplayHook(hwc2_device_t *dev, hwc2_display_t id, Args... args) {
        HWComposer* hwc = static_cast<HWComposer*>(dev);
        if (hwc == nullptr) {
            DLOGW("hwc nullptr: %p", hwc);
        }
        std::shared_ptr<Display> display = (hwc == nullptr ? nullptr : hwc->getDisplay(id));
        Display* displayptr = display.get();
#ifdef WRITE_BACK_MODE
        std::shared_ptr<WriteBackManager> wbMng = WriteBackManager::getWbInstance();
        if (std::is_same<decltype(&Display::validateDisplay), HookType>::value
            && wbMng->isSupportWb()) {
            display = wbMng->handleValidate(display);
        } else if (std::is_same<decltype(&Display::presentDisplay), HookType>::value
                   && wbMng->isSupportWb()) {
            display = wbMng->handlePresent(display);
        }
        if (display != nullptr) {
            displayptr = display.get();
        }
#endif
        if (display == nullptr) {
            if (!hwc->mValidDisplayHandles.count(id))
                return HWC2_ERROR_BAD_DISPLAY;
            return HWC2_ERROR_NONE;
        }
        return static_cast<int32_t>((displayptr->*func)(std::forward<Args>(args)...));
    }

    template <typename HookType, HookType func, typename... Args>
    static int32_t LayerHook(hwc2_device_t *dev, hwc2_display_t id,
            hwc2_layer_t handle, Args... args) {
        HWComposer* hwc = static_cast<HWComposer*>(dev);
        std::shared_ptr<Display> display = hwc->getDisplay(id);
        if (display == nullptr) return HWC2_ERROR_NONE;
        std::shared_ptr<Layer> layer = display->getLayer(handle);
        if (layer) {
            Layer* l = layer.get();
            return static_cast<int32_t>((l->*func)(std::forward<Args>(args)...));
        }
        else
            return HWC2_ERROR_BAD_LAYER;
    }

private:
    mutable std::mutex mDisplayMutex;
    std::map<hwc2_display_t, std::shared_ptr<Display>> mDisplays;
    // Hwc hal will destroy Display object immediately after receiving plug out event,
    // But SurfaceFlinger will call into us after that, so we need this mValidDisplayHandles
    // to identify which display handle is valid.
    std::set<hwc2_display_t> mValidDisplayHandles;

    struct CallbackInfo {
        hwc2_callback_data_t data;
        hwc2_function_pointer_t pointer;
        CallbackInfo(hwc2_callback_data_t d, hwc2_function_pointer_t p)
            : data(d), pointer(p) { }
    };
    std::map<HWC2::Callback, std::unique_ptr<CallbackInfo>> mCallbacks;
    mutable std::mutex mCallbackMutex;
    mutable std::condition_variable mCondition;
    std::vector<std::pair<hwc2_display_t, int>> mPendingHotplugs;

    std::shared_ptr<IDeviceFactory> mDevicePool;
    std::shared_ptr<DeviceManager> mDeviceManager;
    std::unique_ptr<DevicePowerManagerBase> mPowerManager;
};

}

