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

#define LOG_TAG "Operations"

#include "LegacyUtils.h"
#include "OperationResolver.h"
#include "OperationsUtils.h"

namespace android {
namespace nn {
namespace rank_op {

constexpr uint32_t kNumInputs = 1;
constexpr uint32_t kInputTensor = 0;

constexpr uint32_t kNumOutputs = 1;
constexpr uint32_t kOutputScalar = 0;

Result<Version> validate(const IOperationValidationContext* context) {
    NN_RET_CHECK_EQ(context->getNumInputs(), kNumInputs);
    NN_RET_CHECK_EQ(context->getNumOutputs(), kNumOutputs);
    OperandType inputType = context->getInputType(kInputTensor);
    NN_RET_CHECK(inputType == OperandType::TENSOR_FLOAT16 ||
                 inputType == OperandType::TENSOR_FLOAT32 ||
                 inputType == OperandType::TENSOR_INT32 ||
                 inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                 inputType == OperandType::TENSOR_QUANT16_SYMM ||
                 inputType == OperandType::TENSOR_BOOL8 ||
                 inputType == OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL ||
                 inputType == OperandType::TENSOR_QUANT16_ASYMM ||
                 inputType == OperandType::TENSOR_QUANT8_SYMM ||
                 inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED)
            << "Incorrect input type for a RANK op: " << inputType;
    NN_RET_CHECK(validateOutputTypes(context, {OperandType::INT32}));
    return Version::ANDROID_R;
}

bool prepare(IOperationExecutionContext* context) {
    Shape output = context->getOutputShape(kOutputScalar);
    return context->setOutputShape(kOutputScalar, output);
}

bool execute(IOperationExecutionContext* context) {
    *context->getOutputBuffer<int32_t>(kOutputScalar) =
            getNumberOfDimensions(context->getInputShape(kInputTensor));
    return true;
}

}  // namespace rank_op

NN_REGISTER_OPERATION(RANK, "RANK", rank_op::validate, rank_op::prepare, rank_op::execute);

}  // namespace nn
}  // namespace android
