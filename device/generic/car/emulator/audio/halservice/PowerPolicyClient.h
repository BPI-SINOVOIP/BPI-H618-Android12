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

#ifndef AUTOMOTIVE_AUDIOCONTROL_AIDL_DEFAULT_POWERPOLICYCLIENT_H_
#define AUTOMOTIVE_AUDIOCONTROL_AIDL_DEFAULT_POWERPOLICYCLIENT_H_

#include "PowerPolicyClientBase.h"

#include <memory>

namespace aidl {
namespace android {
namespace hardware {
namespace automotive {
namespace audiocontrol {

class AudioControl;

class PowerPolicyClient
    : public ::android::frameworks::automotive::powerpolicy::PowerPolicyClientBase {
  public:
    explicit PowerPolicyClient(const std::shared_ptr<AudioControl>& audioControl);

    void onInitFailed();
    std::vector<::aidl::android::frameworks::automotive::powerpolicy::PowerComponent>
    getComponentsOfInterest() override;
    ::ndk::ScopedAStatus onPolicyChanged(
            const ::aidl::android::frameworks::automotive::powerpolicy::CarPowerPolicy&) override;

  private:
    std::shared_ptr<AudioControl> mAudioControl;
};

}  // namespace audiocontrol
}  // namespace automotive
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // AUTOMOTIVE_AUDIOCONTROL_AIDL_DEFAULT_POWERPOLICYCLIENT_H_
