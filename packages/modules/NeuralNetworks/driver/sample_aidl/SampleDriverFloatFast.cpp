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

#define LOG_TAG "SampleDriverFloatFast"

#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <hidl/LegacySupport.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <memory>
#include <thread>
#include <vector>

#include "AidlHalUtils.h"
#include "SampleDriverPartial.h"

namespace android {
namespace nn {
namespace sample_driver {

class SampleDriverFloatFast : public SampleDriverPartial {
   public:
    SampleDriverFloatFast() : SampleDriverPartial("nnapi-sample_float_fast") {}
    ndk::ScopedAStatus getCapabilities(aidl_hal::Capabilities* capabilities) override;

   private:
    std::vector<bool> getSupportedOperationsImpl(const Model& model) const override;
};

ndk::ScopedAStatus SampleDriverFloatFast::getCapabilities(aidl_hal::Capabilities* capabilities) {
    android::nn::initVLogMask();
    VLOG(DRIVER) << "getCapabilities()";

    *capabilities = {
            .relaxedFloat32toFloat16PerformanceScalar = {.execTime = 0.7f, .powerUsage = 1.1f},
            .relaxedFloat32toFloat16PerformanceTensor = {.execTime = 0.7f, .powerUsage = 1.1f},
            .operandPerformance = nonExtensionOperandPerformance({1.0f, 1.0f}),
            .ifPerformance = {.execTime = 1.0f, .powerUsage = 1.0f},
            .whilePerformance = {.execTime = 1.0f, .powerUsage = 1.0f}};
    update(&capabilities->operandPerformance, aidl_hal::OperandType::TENSOR_FLOAT32,
           {.execTime = 0.8f, .powerUsage = 1.2f});
    update(&capabilities->operandPerformance, aidl_hal::OperandType::FLOAT32,
           {.execTime = 0.8f, .powerUsage = 1.2f});

    return ndk::ScopedAStatus::ok();
}

std::vector<bool> SampleDriverFloatFast::getSupportedOperationsImpl(const Model& model) const {
    const size_t count = model.main.operations.size();
    std::vector<bool> supported(count);
    for (size_t i = 0; i < count; i++) {
        const Operation& operation = model.main.operations[i];
        if (!isExtensionOperationType(operation.type) && operation.inputs.size() > 0) {
            const Operand& firstOperand = model.main.operands[operation.inputs[0]];
            supported[i] = firstOperand.type == OperandType::TENSOR_FLOAT32;
        }
    }
    return supported;
}

}  // namespace sample_driver
}  // namespace nn
}  // namespace android

using android::nn::sample_driver::SampleDriverFloatFast;

int main() {
    std::shared_ptr<SampleDriverFloatFast> driver =
            ndk::SharedRefBase::make<SampleDriverFloatFast>();
    return driver->run();
}
