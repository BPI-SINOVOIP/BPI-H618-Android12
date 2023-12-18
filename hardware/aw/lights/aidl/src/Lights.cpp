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

#include "Lights.h"

#include <android-base/logging.h>
#include <log/log.h>
#include <error.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

#define MODULE_VERSION "V2.0.0"

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    LOG(INFO) << "Lights setting state for id=" << id << " to color " << std::hex << state.color;

    auto it = mLights.find(id);
    if (it == mLights.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    DeviceInfo* hwLight = it->second;
    light_state_t legacyState {
        .color = static_cast<unsigned int>(state.color),
        .flashMode = static_cast<int>(state.flashMode),
        .flashOnMS = state.flashOnMs,
        .flashOffMS = state.flashOffMs,
        .brightnessMode = static_cast<int>(state.brightnessMode),
    };
    int ret = hwLight->set_light(hwLight, &legacyState);
    switch (ret) {
        case 0:
            return ndk::ScopedAStatus::ok();
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    std::vector<HwLight>::iterator it;
    for(it=mLightTypes.begin(); it!=mLightTypes.end(); it++) {
        lights->push_back(*it);
    }
    LOG(INFO) << "Lights reporting supported lights size=" << lights->size();

    return ndk::ScopedAStatus::ok();
}

Lights::Lights() {
    LOG(INFO) << "Light Hal current version " << MODULE_VERSION;
    int id = 0;
    DeviceInfo* lightDevice;
    int minType = (int)LightType::BACKLIGHT;
    //for now, only support backlight
    //int maxType = (int)LightType::MICROPHONE;
    //for (type = minType; type <= maxType; type++){
    for (int type = minType; type <= minType; type++){
        int ret = LightsDevice::open_lights(type, &lightDevice);
        if (ret != 0) {
            ALOGE("open_lights %d failed: %d", type, ret);
        } else {
            mLights[id] = lightDevice;
            HwLight hwLight;
            hwLight.id = id;
            hwLight.ordinal = 0;    //todo: for now only one light for the same logical type
            hwLight.type = (LightType)type;
            mLightTypes.push_back(hwLight);
            id++;
        }
    }

    if (mLights.size() == 0) {
        // Log information, but still return new Light.
        // Some devices may not have any lights.
        ALOGI("Could not open any lights.");
    }
}

Lights::~Lights() {
    ALOGD("~Lights");
    for(auto const &pair : mLights) {
        DeviceInfo* light = pair.second;
        light->close(light);
        light = NULL;
    }
    mLights.clear();
    mLightTypes.clear();
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
