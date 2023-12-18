/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "RADIO_CONFIG"

#include "RadioConfig.h"
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_1 {
namespace implementation {

using namespace ::android::hardware::radio::config::V1_1;

// Methods from ::android::hardware::radio::config::V1_0::IRadioConfig follow.
Return<void> RadioConfig::setResponseFunctions(
    const sp<V1_0::IRadioConfigResponse>& radioConfigResponse,
    const sp<V1_0::IRadioConfigIndication>& radioConfigIndication) {
    mRadioConfigResponse = radioConfigResponse;
    mRadioConfigIndication = radioConfigIndication;

    mRadioConfigResponseV1_1 = V1_1::IRadioConfigResponse::castFrom(radioConfigResponse).withDefault(nullptr);
    mRadioConfigIndicationV1_1 = V1_1::IRadioConfigIndication::castFrom(radioConfigIndication).withDefault(nullptr);
    if (mRadioConfigResponseV1_1 == nullptr)
        RLOGD("mRadioConfigResponseV1_1 == NULL.");
    if (mRadioConfigIndicationV1_1 == nullptr)
        RLOGD("mRadioConfigIndicationV1_1 == NULL.");

    return Void();
}

Return<void> RadioConfig::getSimSlotsStatus(int32_t serial) {
    hidl_vec<V1_0::SimSlotStatus> slotStatus;
    ::android::hardware::radio::V1_0::RadioResponseInfo info;

    info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = ::android::hardware::radio::V1_0::RadioError::REQUEST_NOT_SUPPORTED;

    mRadioConfigResponse->getSimSlotsStatusResponse(info, slotStatus);
    return Void();
}

Return<void> RadioConfig::setSimSlotsMapping(int32_t serial,
                                             const hidl_vec<uint32_t>& /* slotMap */) {
    ::android::hardware::radio::V1_0::RadioResponseInfo info;

    info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = ::android::hardware::radio::V1_0::RadioError::REQUEST_NOT_SUPPORTED;

    mRadioConfigResponse->setSimSlotsMappingResponse(info);
    return Void();
}

Return<void> RadioConfig::getPhoneCapability(int32_t serial) {
    ::android::hardware::radio::V1_0::RadioResponseInfo info;
    const android::hardware::radio::config::V1_1::PhoneCapability phoneCapability = {};

    info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = ::android::hardware::radio::V1_0::RadioError::NONE;

    if (mRadioConfigResponseV1_1 != NULL) {
        mRadioConfigResponseV1_1->getPhoneCapabilityResponse(info, phoneCapability);
        return Void();
    }
    RLOGE("mRadioConfigResponseV1_1 == NULL.");
    return Void();
}

Return<void> RadioConfig::setPreferredDataModem(int32_t serial,
                                                uint8_t modemId) {
    ::android::hardware::radio::V1_0::RadioResponseInfo info;

    info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = ::android::hardware::radio::V1_0::RadioError::NONE;

    if ((int8_t)modemId < 0)
        info.error = ::android::hardware::radio::V1_0::RadioError::INVALID_ARGUMENTS;

    if (mRadioConfigResponseV1_1 != NULL) {
        mRadioConfigResponseV1_1->setPreferredDataModemResponse(info);
        return Void();
    }
    RLOGE("mRadioConfigResponseV1_1 == NULL.");
    return Void();
}

Return<void> RadioConfig::getModemsConfig(int32_t serial) {
    ::android::hardware::radio::V1_0::RadioResponseInfo info;
    const ModemsConfig config = {};

    info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = ::android::hardware::radio::V1_0::RadioError::REQUEST_NOT_SUPPORTED;

    if (mRadioConfigResponseV1_1 != NULL) {
        mRadioConfigResponseV1_1->getModemsConfigResponse(info, config);
        return Void();
    }
    RLOGE("mRadioConfigResponseV1_1 == NULL.");
    return Void();
}

Return<void> RadioConfig::setModemsConfig(int32_t serial,
    const ::android::hardware::radio::config::V1_1::ModemsConfig& /* modemsConfig */) {
    ::android::hardware::radio::V1_0::RadioResponseInfo info;

    info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = ::android::hardware::radio::V1_0::RadioError::REQUEST_NOT_SUPPORTED;

    if (mRadioConfigResponseV1_1 != NULL) {
        mRadioConfigResponseV1_1->setModemsConfigResponse(info);
        return Void();
    }
    RLOGE("mRadioConfigResponseV1_1 == NULL.");
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
