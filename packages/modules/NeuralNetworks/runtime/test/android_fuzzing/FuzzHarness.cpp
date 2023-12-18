/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <LegacyUtils.h>
#include <nnapi/TypeUtils.h>
#include <src/libfuzzer/libfuzzer_macro.h>

#include <algorithm>

#include "Converter.h"
#include "Model.pb.h"
#include "TestHarness.h"

// Fuzz test logic. This function will either run to completion and return, or crash.
extern void nnapiFuzzTest(const ::test_helper::TestModel& testModel);

namespace {

using ::android::nn::getNonExtensionSize;
using ::android::nn::OperandType;
using ::android::nn::fuzz::convertToTestModel;
using ::test_helper::TestModel;
using ::test_helper::TestOperand;

bool operandOverflows(const TestOperand& operand) {
    const auto operandType = static_cast<OperandType>(operand.type);
    return getNonExtensionSize(operandType, operand.dimensions).has_value();
}

bool shouldSkip(const TestModel& model) {
    return std::any_of(model.main.operands.begin(), model.main.operands.end(), operandOverflows);
}

}  // namespace

DEFINE_PROTO_FUZZER(const ::android_nn_fuzz::Test& model) {
    const TestModel testModel = convertToTestModel(model);
    if (!shouldSkip(testModel)) {
        nnapiFuzzTest(testModel);
    }
}
