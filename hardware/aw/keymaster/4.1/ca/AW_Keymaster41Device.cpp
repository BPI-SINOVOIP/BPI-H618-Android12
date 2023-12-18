/*
 **
 ** Copyright 2016, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "android.hardware.keymaster@4.1-impl-aw"

#include "AW_Keymaster41Device.h"

#include <android/log.h>
#include <log/log.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/android_keymaster_messages.h>
#include <keymaster/keymaster_configuration.h>
#include <tee_client_api.h>
#include <hardware/hw_auth_token.h>
#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>
#include <stdio.h>

#include <AW_KM_utility/AWKeymasterLogger.h>
#include <AW_KM_utility/AWTACommunicator.h>

using ::keymaster::ConfigureRequest;
using ::keymaster::ConfigureResponse;
using ::keymaster::DeviceLockedRequest;
using ::keymaster::DeviceLockedResponse;
using ::keymaster::EarlyBootEndedResponse;

namespace aw {
namespace hardware {
namespace keymaster {
namespace V4_1 {
namespace implementation {

namespace {

using ::android::hardware::keymaster::V4_0::Tag;
using V4_0::KmParamSet;

inline V41ErrorCode legacy_enum_conversion(const keymaster_error_t value) {
    return static_cast<V41ErrorCode>(value);
}

inline keymaster_security_level_t legacy_enum_conversion(SecurityLevel value) {
    return static_cast<keymaster_security_level_t>(value);
}

enum {
    MSG_KEYMASTER_EARLY_BOOT_ENDED = 0x237,
    MSG_KEYMASTER_DEVICE_LOCKED = 0x238,
};

static const uint8_t KeyMaster_v41_UUID[16] = {0x7B, 0x01, 0x3D, 0x66, 0x2D, 0x10, 0xE0, 0x4F,
                                              0xC0, 0x86, 0x52, 0x3E, 0x1C, 0x75, 0x48, 0x46};

}  // anonymous namespace

AndroidKeymaster41Device_AW::AndroidKeymaster41Device_AW()
    : aw::hardware::keymaster::V4_0::implementation::AndroidKeymaster4Device_AW(
          "AW_keymaster_4.1") {
    ta_forward_ = nullptr;
}

AndroidKeymaster41Device_AW::~AndroidKeymaster41Device_AW() {
    delete ta_forward_;
}

int AndroidKeymaster41Device_AW::SetUpTa() {
    if (ta_forward_ == nullptr) ta_forward_ = new AWTACommunicator(logger_);
    return ta_forward_->LoadTa(&KeyMaster_v41_UUID[0]);
}

Return<V41ErrorCode>
AndroidKeymaster41Device_AW::deviceLocked(bool passwordOnly,
                                          const VerificationToken& verificationToken) {
    DeviceLockedRequest request(::keymaster::kDefaultMessageVersion);
    DeviceLockedResponse response(::keymaster::kDefaultMessageVersion);

    request.passwordOnly = passwordOnly;
    request.token.challenge = verificationToken.challenge;
    request.token.timestamp = verificationToken.timestamp;
    request.token.security_level = legacy_enum_conversion(verificationToken.securityLevel);
    request.token.parameters_verified.Reinitialize(
        KmParamSet(verificationToken.parametersVerified));
    request.token.mac =
        ::keymaster::KeymasterBlob(verificationToken.mac.data(), verificationToken.mac.size());

    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_DEVICE_LOCKED, request, &response);
    return legacy_enum_conversion(response.error);
}

Return<V41ErrorCode> AndroidKeymaster41Device_AW::earlyBootEnded() {
    EarlyBootEndedResponse response(::keymaster::kDefaultMessageVersion);

    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_EARLY_BOOT_ENDED, &response);
    return legacy_enum_conversion(response.error);
}

}  // namespace implementation
}  // namespace V4_1
}  // namespace keymaster
}  // namespace hardware
}  // namespace aw
