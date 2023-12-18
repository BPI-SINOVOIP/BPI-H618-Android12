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

#define LOG_TAG "SampleDriverQuant"

#include <android-base/logging.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <memory>
#include <thread>
#include <vector>

#include "AidlHalUtils.h"
#include "SampleDriverPartial.h"

namespace android {
namespace nn {
namespace sample_driver {

class SampleDriverQuant : public SampleDriverPartial {
   public:
    SampleDriverQuant() : SampleDriverPartial("nnapi-sample_quant") {}
    ndk::ScopedAStatus getCapabilities(aidl_hal::Capabilities* capabilities) override;

   private:
    std::vector<bool> getSupportedOperationsImpl(const Model& model) const override;
};

ndk::ScopedAStatus SampleDriverQuant::getCapabilities(aidl_hal::Capabilities* capabilities) {
    android::nn::initVLogMask();
    VLOG(DRIVER) << "getCapabilities()";

    *capabilities = {
            .relaxedFloat32toFloat16PerformanceScalar = {.execTime = 50.0f, .powerUsage = 1.0f},
            .relaxedFloat32toFloat16PerformanceTensor = {.execTime = 50.0f, .powerUsage = 1.0f},
            .operandPerformance = nonExtensionOperandPerformance({50.0f, 1.0f}),
            .ifPerformance = {.execTime = 50.0f, .powerUsage = 1.0f},
            .whilePerformance = {.execTime = 50.0f, .powerUsage = 1.0f}};

    return ndk::ScopedAStatus::ok();
}

static bool isQuantized(OperandType opType) {
    return opType == OperandType::TENSOR_QUANT8_ASYMM ||
           opType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED;
}

std::vector<bool> SampleDriverQuant::getSupportedOperationsImpl(const Model& model) const {
    const size_t count = model.main.operations.size();
    std::vector<bool> supported(count);
    for (size_t i = 0; i < count; i++) {
        const Operation& operation = model.main.operations[i];
        if (!isExtensionOperationType(operation.type) && operation.inputs.size() > 0) {
            const Operand& firstOperand = model.main.operands[operation.inputs[0]];
            supported[i] = isQuantized(firstOperand.type);
            if (operation.type == OperationType::SELECT) {
                const Operand& secondOperand = model.main.operands[operation.inputs[1]];
                supported[i] = isQuantized(secondOperand.type);
            }
        }
    }
    return supported;
}

}  // namespace sample_driver
}  // namespace nn
}  // namespace android

using android::nn::sample_driver::SampleDriverQuant;

int main() {
    std::shared_ptr<SampleDriverQuant> driver = ndk::SharedRefBase::make<SampleDriverQuant>();
    return driver->run();
}
