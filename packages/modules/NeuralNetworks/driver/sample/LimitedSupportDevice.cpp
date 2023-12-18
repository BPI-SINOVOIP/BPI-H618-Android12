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

#include "LimitedSupportDevice.h"

#include <android-base/logging.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>

#include <algorithm>
#include <any>
#include <chrono>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace android::nn::sample {

LimitedSupportDevice::LimitedSupportDevice(SharedDevice device, Capabilities capabilities,
                                           SupportedOperationsFunction supportedOperationsFunction)
    : kDevice(std::move(device)),
      kCapabilities(std::move(capabilities)),
      kSupportedOperationsFunction(std::move(supportedOperationsFunction)) {
    CHECK(kDevice != nullptr);
    CHECK(kSupportedOperationsFunction != nullptr);
    const auto result = validate(kCapabilities);
    CHECK(result.has_value()) << result.error();
}

const std::string& LimitedSupportDevice::getName() const {
    return kDevice->getName();
}

const std::string& LimitedSupportDevice::getVersionString() const {
    return kDevice->getVersionString();
}

Version LimitedSupportDevice::getFeatureLevel() const {
    return kDevice->getFeatureLevel();
}

DeviceType LimitedSupportDevice::getType() const {
    return kDevice->getType();
}

const std::vector<Extension>& LimitedSupportDevice::getSupportedExtensions() const {
    return kDevice->getSupportedExtensions();
}

const Capabilities& LimitedSupportDevice::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> LimitedSupportDevice::getNumberOfCacheFilesNeeded() const {
    return kDevice->getNumberOfCacheFilesNeeded();
}

GeneralResult<void> LimitedSupportDevice::wait() const {
    return kDevice->wait();
}

GeneralResult<std::vector<bool>> LimitedSupportDevice::getSupportedOperations(
        const Model& model) const {
    return kSupportedOperationsFunction(model);
}

GeneralResult<SharedPreparedModel> LimitedSupportDevice::prepareModel(
        const Model& model, ExecutionPreference preference, Priority priority,
        OptionalTimePoint deadline, const std::vector<SharedHandle>& modelCache,
        const std::vector<SharedHandle>& dataCache, const CacheToken& token) const {
    const auto supportedOperations = NN_TRY(kSupportedOperationsFunction(model));
    constexpr auto id = [](auto v) { return v; };
    if (!std::all_of(supportedOperations.begin(), supportedOperations.end(), id)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Not all operations are supported";
    }
    return kDevice->prepareModel(model, preference, priority, deadline, modelCache, dataCache,
                                 token);
}

GeneralResult<SharedPreparedModel> LimitedSupportDevice::prepareModelFromCache(
        OptionalTimePoint deadline, const std::vector<SharedHandle>& modelCache,
        const std::vector<SharedHandle>& dataCache, const CacheToken& token) const {
    return kDevice->prepareModelFromCache(deadline, modelCache, dataCache, token);
}

GeneralResult<SharedBuffer> LimitedSupportDevice::allocate(
        const BufferDesc& desc, const std::vector<SharedPreparedModel>& preparedModels,
        const std::vector<BufferRole>& inputRoles,
        const std::vector<BufferRole>& outputRoles) const {
    return kDevice->allocate(desc, preparedModels, inputRoles, outputRoles);
}

}  // namespace android::nn::sample
