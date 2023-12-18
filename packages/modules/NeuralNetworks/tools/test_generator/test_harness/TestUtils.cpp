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

#include <android-base/logging.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/Types.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "TestHarness.h"

namespace android::nn::test {
namespace {

using ::test_helper::TestBuffer;
using ::test_helper::TestModel;
using ::test_helper::TestOperand;
using ::test_helper::TestOperandLifeTime;
using ::test_helper::TestOperandType;
using ::test_helper::TestOperation;
using ::test_helper::TestSubgraph;

Operand createOperand(const TestOperand& operand, Model::OperandValues* operandValues,
                      ConstantMemoryBuilder* memoryBuilder) {
    CHECK(operandValues != nullptr);
    CHECK(memoryBuilder != nullptr);

    const OperandType type = static_cast<OperandType>(operand.type);
    const Operand::LifeTime lifetime = static_cast<Operand::LifeTime>(operand.lifetime);

    DataLocation location;
    switch (operand.lifetime) {
        case TestOperandLifeTime::TEMPORARY_VARIABLE:
        case TestOperandLifeTime::SUBGRAPH_INPUT:
        case TestOperandLifeTime::SUBGRAPH_OUTPUT:
        case TestOperandLifeTime::NO_VALUE:
            break;
        case TestOperandLifeTime::CONSTANT_COPY:
            location = operandValues->append(operand.data.get<uint8_t>(), operand.data.size());
            break;
        case TestOperandLifeTime::CONSTANT_REFERENCE:
            location = memoryBuilder->append(operand.data.get<void>(), operand.data.size());
            break;
        case TestOperandLifeTime::SUBGRAPH:
            location = {.offset = *operand.data.get<uint32_t>()};
            break;
    }

    Operand::ExtraParams extraParams;
    if (operand.type == TestOperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
        extraParams =
                Operand::SymmPerChannelQuantParams{.scales = operand.channelQuant.scales,
                                                   .channelDim = operand.channelQuant.channelDim};
    }

    return {
            .type = type,
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = lifetime,
            .location = location,
            .extraParams = std::move(extraParams),
    };
}

Model::Subgraph createSubgraph(const TestSubgraph& testSubgraph,
                               Model::OperandValues* operandValues,
                               ConstantMemoryBuilder* memoryBuilder) {
    // Operands.
    std::vector<Operand> operands;
    operands.reserve(testSubgraph.operands.size());
    std::transform(testSubgraph.operands.begin(), testSubgraph.operands.end(),
                   std::back_inserter(operands),
                   [operandValues, memoryBuilder](const TestOperand& operand) {
                       return createOperand(operand, operandValues, memoryBuilder);
                   });

    // Operations.
    std::vector<Operation> operations;
    operations.reserve(testSubgraph.operations.size());
    std::transform(testSubgraph.operations.begin(), testSubgraph.operations.end(),
                   std::back_inserter(operations), [](const TestOperation& op) -> Operation {
                       return {.type = static_cast<OperationType>(op.type),
                               .inputs = op.inputs,
                               .outputs = op.outputs};
                   });

    return {.operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = testSubgraph.inputIndexes,
            .outputIndexes = testSubgraph.outputIndexes};
}

}  // namespace

Model createModel(const TestModel& testModel) {
    Model::OperandValues operandValues;
    ConstantMemoryBuilder memoryBuilder(0);

    Model::Subgraph mainSubgraph = createSubgraph(testModel.main, &operandValues, &memoryBuilder);
    std::vector<Model::Subgraph> refSubgraphs;
    refSubgraphs.reserve(testModel.referenced.size());
    std::transform(testModel.referenced.begin(), testModel.referenced.end(),
                   std::back_inserter(refSubgraphs),
                   [&operandValues, &memoryBuilder](const TestSubgraph& testSubgraph) {
                       return createSubgraph(testSubgraph, &operandValues, &memoryBuilder);
                   });

    // Shared memory.
    std::vector<SharedMemory> pools;
    if (!memoryBuilder.empty()) {
        pools.push_back(memoryBuilder.finish().value());
    }

    return {.main = std::move(mainSubgraph),
            .referenced = std::move(refSubgraphs),
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
            .relaxComputationFloat32toFloat16 = testModel.isRelaxed};
}

Request createRequest(const TestModel& testModel) {
    constexpr uint32_t kInputPoolIndex = 0;
    constexpr uint32_t kOutputPoolIndex = 1;

    // Model inputs.
    std::vector<Request::Argument> inputs;
    inputs.reserve(testModel.main.inputIndexes.size());
    ConstantMemoryBuilder inputBuilder(kInputPoolIndex);
    for (uint32_t operandIndex : testModel.main.inputIndexes) {
        const auto& op = testModel.main.operands[operandIndex];
        Request::Argument requestArgument;
        if (op.data.size() == 0) {
            // Omitted input.
            requestArgument = {.lifetime = Request::Argument::LifeTime::NO_VALUE};
        } else {
            const DataLocation location = inputBuilder.append(op.data.get<void>(), op.data.size());
            requestArgument = {.lifetime = Request::Argument::LifeTime::POOL,
                               .location = location,
                               .dimensions = op.dimensions};
        }
        inputs.push_back(std::move(requestArgument));
    }

    // Model outputs.
    std::vector<Request::Argument> outputs;
    outputs.reserve(testModel.main.outputIndexes.size());
    MutableMemoryBuilder outputBuilder(kOutputPoolIndex);
    for (uint32_t operandIndex : testModel.main.outputIndexes) {
        const auto& op = testModel.main.operands[operandIndex];

        // In the case of zero-sized output, we should at least provide a one-byte buffer.
        // This is because zero-sized tensors are only supported internally to the driver, or
        // reported in output shapes. It is illegal for the client to pre-specify a zero-sized
        // tensor as model output. Otherwise, we will have two semantic conflicts:
        // - "Zero dimension" conflicts with "unspecified dimension".
        // - "Omitted operand buffer" conflicts with "zero-sized operand buffer".
        size_t bufferSize = std::max<size_t>(op.data.size(), 1);

        const DataLocation location = outputBuilder.append(bufferSize);
        outputs.push_back({.lifetime = Request::Argument::LifeTime::POOL,
                           .location = location,
                           .dimensions = op.dimensions});
    }

    // Model pools.
    auto inputMemory = inputBuilder.finish().value();
    auto outputMemory = outputBuilder.finish().value();
    std::vector<Request::MemoryPool> pools = {std::move(inputMemory), std::move(outputMemory)};

    return {.inputs = std::move(inputs), .outputs = std::move(outputs), .pools = std::move(pools)};
}

}  // namespace android::nn::test
