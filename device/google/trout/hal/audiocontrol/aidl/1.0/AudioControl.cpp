/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "AudioControl.h"

#include <aidl/android/hardware/automotive/audiocontrol/AudioFocusChange.h>
#include <aidl/android/hardware/automotive/audiocontrol/DuckingInfo.h>
#include <aidl/android/hardware/automotive/audiocontrol/IFocusListener.h>

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <android_audio_policy_configuration_V7_0.h>
#include <private/android_filesystem_config.h>

#include <stdio.h>

namespace aidl::android::hardware::automotive::audiocontrol {

using ::android::base::EqualsIgnoreCase;
using ::android::base::ParseInt;
using ::std::string;

namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

AudioControl::AudioControl(const std::string& audio_control_server_addr)
    : mAudioControlServer(MakeAudioControlServer(audio_control_server_addr)) {}

ndk::ScopedAStatus AudioControl::registerFocusListener(
        const shared_ptr<IFocusListener>& in_listener) {
    LOG(DEBUG) << "registering focus listener";

    if (in_listener) {
        mAudioControlServer->RegisterFocusListener(mFocusListener = in_listener);
    } else {
        LOG(ERROR) << "Unexpected nullptr for listener resulting in no-op.";
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::setBalanceTowardRight(float value) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::setFadeTowardFront(float value) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::onAudioFocusChange(const string& in_usage, int32_t in_zoneId,
                                                    AudioFocusChange in_focusChange) {
    LOG(INFO) << "Focus changed: " << toString(in_focusChange).c_str() << " for usage "
              << in_usage.c_str() << " in zone " << in_zoneId;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::onDevicesToDuckChange(
        const std::vector<DuckingInfo>& in_duckingInfos) {
    LOG(INFO) << "AudioControl::onDevicesToDuckChange";
    for (const DuckingInfo& duckingInfo : in_duckingInfos) {
        LOG(INFO) << "zone: " << duckingInfo.zoneId;
        LOG(INFO) << "Devices to duck:";
        for (const auto& addressToDuck : duckingInfo.deviceAddressesToDuck) {
            LOG(INFO) << addressToDuck;
        }
        LOG(INFO) << "Devices to unduck:";
        for (const auto& addressToUnduck : duckingInfo.deviceAddressesToUnduck) {
            LOG(INFO) << addressToUnduck;
        }
        LOG(INFO) << "Usages holding focus:";
        for (const auto& usage : duckingInfo.usagesHoldingFocus) {
            LOG(INFO) << usage;
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::onDevicesToMuteChange(
        const std::vector<MutingInfo>& in_mutingInfos) {
    LOG(INFO) << "AudioControl::onDevicesToMuteChange";
    for (const MutingInfo& mutingInfo : in_mutingInfos) {
        LOG(INFO) << "zone: " << mutingInfo.zoneId;
        LOG(INFO) << "Devices to mute:";
        for (const auto& addressToMute : mutingInfo.deviceAddressesToMute) {
            LOG(INFO) << addressToMute;
        }
        LOG(INFO) << "Devices to unmute:";
        for (const auto& addressToUnmute : mutingInfo.deviceAddressesToUnmute) {
            LOG(INFO) << addressToUnmute;
        }
    }
    return ndk::ScopedAStatus::ok();
}

binder_status_t AudioControl::dump(int fd, const char** args, uint32_t numArgs) {
    return STATUS_BAD_VALUE;
}

bool AudioControl::isHealthy() {
    // TODO(egranata, chenhaosjtuacm): fill this in with a real check
    // e.g. add a heartbeat message to remote side
    return true;
}

}  // namespace aidl::android::hardware::automotive::audiocontrol
