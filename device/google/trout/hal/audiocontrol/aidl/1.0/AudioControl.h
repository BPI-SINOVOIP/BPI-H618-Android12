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
#ifndef ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_AUDIOCONTROL_H
#define ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_AUDIOCONTROL_H

#include "AudioControlServer.h"

#include <aidl/android/hardware/automotive/audiocontrol/AudioFocusChange.h>
#include <aidl/android/hardware/automotive/audiocontrol/BnAudioControl.h>
#include <aidl/android/hardware/automotive/audiocontrol/DuckingInfo.h>

namespace aidl::android::hardware::automotive::audiocontrol {

using ::std::shared_ptr;
using ::std::unique_ptr;

class AudioControl : public BnAudioControl {
  public:
    explicit AudioControl(const std::string& audio_control_server_addr);

    ndk::ScopedAStatus onAudioFocusChange(const std::string& in_usage, int32_t in_zoneId,
                                          AudioFocusChange in_focusChange) override;
    ndk::ScopedAStatus onDevicesToDuckChange(
            const std::vector<DuckingInfo>& in_duckingInfos) override;
    ndk::ScopedAStatus onDevicesToMuteChange(
            const std::vector<MutingInfo>& in_mutingInfos) override;
    ndk::ScopedAStatus registerFocusListener(
            const shared_ptr<IFocusListener>& in_listener) override;
    ndk::ScopedAStatus setBalanceTowardRight(float in_value) override;
    ndk::ScopedAStatus setFadeTowardFront(float in_value) override;
    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    bool isHealthy();

  private:
    // This focus listener will only be used by this HAL instance to communicate with
    // a single instance of CarAudioService. As such, it doesn't have explicit serialization.
    // If a different AudioControl implementation were to have multiple threads leveraging this
    // listener, then it should also include mutexes or make the listener atomic.
    shared_ptr<IFocusListener> mFocusListener;
    unique_ptr<AudioControlServer> mAudioControlServer;
};

}  // namespace aidl::android::hardware::automotive::audiocontrol

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_AUDIOCONTROL_H
