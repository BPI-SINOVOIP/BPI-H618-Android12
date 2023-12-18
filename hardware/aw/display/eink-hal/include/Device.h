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

#ifndef ANDROID_HWC_DEVICE_H_
#define ANDROID_HWC_DEVICE_H_

#include <memory>
#include <string>

namespace sunxi {

struct CompositionContext;

class DeviceBase {
public:
    class EventListener {
    public:
        virtual ~EventListener() = default;
        virtual void onVsync(int64_t timestamp) = 0;
        virtual void onRefresh() = 0;
    };

    struct Config {
        std::string name;
        int configId;
        int width;
        int height;
        int dpix;
        int dpiy;
        /* refreshRate in Hz */
        int refreshRate;
    };

    virtual ~DeviceBase() = default;

    virtual const char* name() = 0;
    virtual const Config& getDefaultConfig() = 0;
    virtual int getRefreshRate() = 0;

    virtual void prepare(CompositionContext *ctx) = 0;
    virtual void commit(CompositionContext *ctx)  = 0;

    virtual int32_t setPowerMode(int32_t mode) = 0;
    virtual void setEventListener(std::weak_ptr<EventListener> listener) = 0;
    virtual void setVsyncEnabled(bool enabled) = 0;
    virtual void skipFrameEnable(bool enabled) = 0;
    virtual void setColorTransform(const float *matrix, int32_t hint) = 0;
    virtual void dump(std::string& out);
};

} // namespace sunxi

#endif
