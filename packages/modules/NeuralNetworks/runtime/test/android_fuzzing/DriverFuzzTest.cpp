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

#include <MemoryUtils.h>
#include <SampleDriverFull.h>
#include <Utils.h>
#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "TestHarness.h"

namespace {

using ::android::hidl::memory::V1_0::IMemory;
using ::test_helper::TestModel;
using namespace test_helper;
using namespace android;
using namespace android::hardware;
namespace V1_0 = neuralnetworks::V1_0;
namespace V1_1 = neuralnetworks::V1_1;
namespace V1_2 = neuralnetworks::V1_2;
namespace V1_3 = neuralnetworks::V1_3;
using V1_0::DataLocation;

sp<V1_3::IDevice> getDevice() {
    /**
     * TODO: INSERT CUSTOM DEVICE HERE
     */
    static const sp<V1_3::IDevice> device = new nn::sample_driver::SampleDriverFull(
            "example-driver", V1_0::PerformanceInfo{.execTime = 1.0f, .powerUsage = 1.0f});
    return device;
}

V1_3::Subgraph createSubgraph(const TestSubgraph& testSubgraph, uint32_t* constCopySize,
                              std::vector<const TestBuffer*>* constCopies, uint32_t* constRefSize,
                              std::vector<const TestBuffer*>* constReferences) {
    CHECK(constCopySize != nullptr);
    CHECK(constCopies != nullptr);
    CHECK(constRefSize != nullptr);
    CHECK(constReferences != nullptr);

    // Operands.
    hidl_vec<V1_3::Operand> operands(testSubgraph.operands.size());
    for (uint32_t i = 0; i < testSubgraph.operands.size(); i++) {
        const auto& op = testSubgraph.operands[i];

        DataLocation loc = {};
        if (op.lifetime == TestOperandLifeTime::CONSTANT_COPY) {
            loc = {
                    .poolIndex = 0,
                    .offset = *constCopySize,
                    .length = static_cast<uint32_t>(op.data.size()),
            };
            constCopies->push_back(&op.data);
            *constCopySize += op.data.alignedSize();
        } else if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
            loc = {
                    .poolIndex = 0,
                    .offset = *constRefSize,
                    .length = static_cast<uint32_t>(op.data.size()),
            };
            constReferences->push_back(&op.data);
            *constRefSize += op.data.alignedSize();
        } else if (op.lifetime == TestOperandLifeTime::SUBGRAPH) {
            loc = {
                    .poolIndex = 0,
                    .offset = *op.data.get<uint32_t>(),
                    .length = 0,
            };
        }

        V1_2::Operand::ExtraParams extraParams;
        if (op.type == TestOperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
            extraParams.channelQuant(V1_2::SymmPerChannelQuantParams{
                    .scales = op.channelQuant.scales, .channelDim = op.channelQuant.channelDim});
        }

        operands[i] = {.type = static_cast<V1_3::OperandType>(op.type),
                       .dimensions = op.dimensions,
                       .numberOfConsumers = op.numberOfConsumers,
                       .scale = op.scale,
                       .zeroPoint = op.zeroPoint,
                       .lifetime = static_cast<V1_3::OperandLifeTime>(op.lifetime),
                       .location = loc,
                       .extraParams = std::move(extraParams)};
    }

    // Operations.
    hidl_vec<V1_3::Operation> operations(testSubgraph.operations.size());
    std::transform(testSubgraph.operations.begin(), testSubgraph.operations.end(),
                   operations.begin(), [](const TestOperation& op) -> V1_3::Operation {
                       return {.type = static_cast<V1_3::OperationType>(op.type),
                               .inputs = op.inputs,
                               .outputs = op.outputs};
                   });

    return {.operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = testSubgraph.inputIndexes,
            .outputIndexes = testSubgraph.outputIndexes};
}

void copyTestBuffers(const std::vector<const TestBuffer*>& buffers, uint8_t* output) {
    uint32_t offset = 0;
    for (const TestBuffer* buffer : buffers) {
        const uint8_t* begin = buffer->get<uint8_t>();
        const uint8_t* end = begin + buffer->size();
        std::copy(begin, end, output + offset);
        offset += buffer->alignedSize();
    }
}

V1_3::Model createModel(const TestModel& testModel) {
    uint32_t constCopySize = 0;
    uint32_t constRefSize = 0;
    std::vector<const TestBuffer*> constCopies;
    std::vector<const TestBuffer*> constReferences;

    V1_3::Subgraph mainSubgraph = createSubgraph(testModel.main, &constCopySize, &constCopies,
                                                 &constRefSize, &constReferences);
    hidl_vec<V1_3::Subgraph> refSubgraphs(testModel.referenced.size());
    std::transform(testModel.referenced.begin(), testModel.referenced.end(), refSubgraphs.begin(),
                   [&constCopySize, &constCopies, &constRefSize,
                    &constReferences](const TestSubgraph& testSubgraph) {
                       return createSubgraph(testSubgraph, &constCopySize, &constCopies,
                                             &constRefSize, &constReferences);
                   });

    // Constant copies.
    hidl_vec<uint8_t> operandValues(constCopySize);
    copyTestBuffers(constCopies, operandValues.data());

    // Shared memory.
    std::vector<hidl_memory> pools = {};
    if (constRefSize > 0) {
        pools.push_back(nn::allocateSharedMemory(constRefSize));
        CHECK_NE(pools.back().size(), 0u);

        // load data
        sp<IMemory> mappedMemory = mapMemory(pools[0]);
        CHECK(mappedMemory.get() != nullptr);
        uint8_t* mappedPtr =
                reinterpret_cast<uint8_t*>(static_cast<void*>(mappedMemory->getPointer()));
        CHECK(mappedPtr != nullptr);

        copyTestBuffers(constReferences, mappedPtr);
    }

    return {.main = std::move(mainSubgraph),
            .referenced = std::move(refSubgraphs),
            .operandValues = std::move(operandValues),
            .pools = pools,
            .relaxComputationFloat32toFloat16 = testModel.isRelaxed};
}

V1_3::Request createRequest(const TestModel& testModel) {
    static constexpr uint32_t kInputPoolIndex = 0;
    static constexpr uint32_t kOutputPoolIndex = 1;

    // Model inputs.
    hidl_vec<V1_0::RequestArgument> inputs(testModel.main.inputIndexes.size());
    size_t inputSize = 0;
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
        if (op.data.size() == 0) {
            // Omitted input.
            inputs[i] = {.hasNoValue = true};
        } else {
            DataLocation loc = {.poolIndex = kInputPoolIndex,
                                .offset = static_cast<uint32_t>(inputSize),
                                .length = static_cast<uint32_t>(op.data.size())};
            inputSize += op.data.alignedSize();
            inputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
        }
    }

    // Model outputs.
    hidl_vec<V1_0::RequestArgument> outputs(testModel.main.outputIndexes.size());
    size_t outputSize = 0;
    for (uint32_t i = 0; i < testModel.main.outputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.outputIndexes[i]];

        // In the case of zero-sized output, we should at least provide a one-byte buffer.
        // This is because zero-sized tensors are only supported internally to the driver, or
        // reported in output shapes. It is illegal for the client to pre-specify a zero-sized
        // tensor as model output. Otherwise, we will have two semantic conflicts:
        // - "Zero dimension" conflicts with "unspecified dimension".
        // - "Omitted operand buffer" conflicts with "zero-sized operand buffer".
        size_t bufferSize = std::max<size_t>(op.data.size(), 1);

        DataLocation loc = {.poolIndex = kOutputPoolIndex,
                            .offset = static_cast<uint32_t>(outputSize),
                            .length = static_cast<uint32_t>(bufferSize)};
        outputSize += op.data.size() == 0 ? TestBuffer::kAlignment : op.data.alignedSize();
        outputs[i] = {.hasNoValue = false, .location = loc, .dimensions = {}};
    }

    // Allocate memory pools.
    inputSize = std::max<size_t>(inputSize, 1);
    auto inputMemory = nn::allocateSharedMemory(inputSize);
    CHECK(inputMemory.valid());
    outputSize = std::max<size_t>(outputSize, 1);
    auto outputMemory = nn::allocateSharedMemory(outputSize);
    CHECK(outputMemory.valid());
    hidl_vec<V1_3::Request::MemoryPool> pools(2);
    pools[kInputPoolIndex].hidlMemory(inputMemory);
    pools[kOutputPoolIndex].hidlMemory(outputMemory);

    // Map input memory pool.
    const auto mappedInput = mapMemory(inputMemory);
    CHECK(mappedInput.get() != nullptr);
    uint8_t* const inputPtr = static_cast<uint8_t*>(static_cast<void*>(mappedInput->getPointer()));
    CHECK(inputPtr != nullptr);

    // Copy input data to the memory pool.
    for (uint32_t i = 0; i < testModel.main.inputIndexes.size(); i++) {
        const auto& op = testModel.main.operands[testModel.main.inputIndexes[i]];
        if (op.data.size() > 0) {
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, inputPtr + inputs[i].location.offset);
        }
    }

    return {.inputs = std::move(inputs), .outputs = std::move(outputs), .pools = std::move(pools)};
}

class PreparedModelCallback : public V1_3::IPreparedModelCallback {
   public:
    Return<void> notify(V1_0::ErrorStatus /*status*/,
                        const sp<V1_0::IPreparedModel>& /*preparedModel*/) override {
        LOG(FATAL) << "not implemented";
        return Void();
    }
    Return<void> notify_1_2(V1_0::ErrorStatus /*status*/,
                            const sp<V1_2::IPreparedModel>& /*preparedModel*/) override {
        LOG(FATAL) << "not implemented";
        return Void();
    }
    Return<void> notify_1_3(V1_3::ErrorStatus status,
                            const sp<V1_3::IPreparedModel>& preparedModel) override {
        const sp<V1_3::IPreparedModel> result =
                (status == V1_3::ErrorStatus::NONE ? preparedModel : nullptr);
        {
            std::lock_guard guard(mMutex);
            if (mResults.has_value()) return Void();
            mResults.emplace(result);
        }
        mCondition.notify_all();
        return Void();
    }

    const sp<V1_3::IPreparedModel>& getResults() const {
        {
            std::unique_lock lock(mMutex);
            mCondition.wait(lock, [this] { return mResults.has_value(); });
        }
        return mResults.value();
    }

   private:
    mutable std::mutex mMutex;
    mutable std::condition_variable mCondition;
    std::optional<const sp<V1_3::IPreparedModel>> mResults;
};

sp<V1_3::IPreparedModel> prepareModel(const sp<V1_3::IDevice>& device, const V1_3::Model& model) {
    const sp<PreparedModelCallback> callback = new PreparedModelCallback();
    device->prepareModel_1_3(model, V1_1::ExecutionPreference::FAST_SINGLE_ANSWER,
                             V1_3::Priority::MEDIUM, {}, {}, {}, {}, callback);
    return callback->getResults();
}

void execute(const sp<V1_3::IPreparedModel>& preparedModel, const V1_3::Request& request) {
    const auto cb = [](V1_3::ErrorStatus /*status*/,
                       const hidl_vec<V1_2::OutputShape>& /*outputShapes*/,
                       V1_2::Timing /*timing*/) {
        // TODO: CHECK VTS requirements?
    };
    preparedModel->executeSynchronously_1_3(request, V1_2::MeasureTiming::YES, {}, {}, cb);
}

}  // anonymous namespace

void nnapiFuzzTest(const TestModel& testModel) {
    // Set up device.
    const auto device = getDevice();
    CHECK(device != nullptr);

    // Set up model.
    const auto model = createModel(testModel);

    // Attempt to prepare the model.
    const auto preparedModel = prepareModel(device, model);
    if (preparedModel == nullptr) return;

    // Set up request.
    const auto request = createRequest(testModel);

    // Perform execution.
    execute(preparedModel, request);
}
