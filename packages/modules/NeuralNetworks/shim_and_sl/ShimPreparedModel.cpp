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

#include "ShimPreparedModel.h"

#include <aidl/android/hardware/neuralnetworks/BnBurst.h>
#include <aidl/android/hardware/neuralnetworks/BnFencedExecutionCallback.h>
#include <aidl/android/hardware/neuralnetworks/ErrorStatus.h>
#include <aidl/android/hardware/neuralnetworks/OutputShape.h>
#include <aidl/android/hardware/neuralnetworks/RequestMemoryPool.h>
#include <android-base/chrono_utils.h>
#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android/binder_auto_utils.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "ShimConverter.h"
#include "ShimUtils.h"

namespace aidl::android::hardware::neuralnetworks {

ErrorStatus ShimPreparedModel::parseInputs(
        const Request& request, bool measure, int64_t deadlineNs, int64_t loopTimeoutDurationNs,
        ::android::nn::sl_wrapper::Execution* execution,
        std::vector<std::shared_ptr<::android::nn::sl_wrapper::Memory>>* requestMemoryPools) {
    for (const auto& requestPool : request.pools) {
        switch (requestPool.getTag()) {
            case RequestMemoryPool::pool: {
                const auto& memoryPool = requestPool.get<RequestMemoryPool::pool>();
                std::shared_ptr<::android::nn::sl_wrapper::Memory> mem =
                        convertFromHAL(mNnapi.get(), memoryPool);
                if (!mem) {
                    LOG(ERROR) << "Failed to convert request HAL memory pools into SL memory";
                    return ErrorStatus::INVALID_ARGUMENT;
                }

                requestMemoryPools->push_back(mem);
                break;
            }
            case RequestMemoryPool::token: {
                int token = requestPool.get<RequestMemoryPool::token>();

                auto memory = mBufferTracker->get(static_cast<uint32_t>(token));
                if (memory == nullptr) {
                    return ErrorStatus::INVALID_ARGUMENT;
                }

                requestMemoryPools->push_back(memory);
                break;
            }
        }
    }

    const auto& model = mMainAndReferencedModels[0];
    // set inputs
    for (int i = 0; i < request.inputs.size(); ++i) {
        const auto& input = request.inputs[i];
        ::android::nn::wrapper::OperandType operandType = model.getOperands()[model.getInputs()[i]];
        if (!input.hasNoValue) {
            if (input.dimensions.size() > 0) {
                operandType.updateDimensions(::android::nn::toUnsigned(input.dimensions).value());
            }
            auto result = execution->setInputFromMemory(
                    i, requestMemoryPools->at(input.location.poolIndex).get(),
                    input.location.offset, input.location.length, &operandType.operandType);
            if (result != Result::NO_ERROR) {
                return convertResultToErrorStatus(result);
            }
        } else {
            auto result = execution->setInput(i, nullptr, 0);
            if (result != Result::NO_ERROR) {
                return convertResultToErrorStatus(result);
            }
        }
    }

    // set outputs
    for (int i = 0; i < request.outputs.size(); ++i) {
        const auto& output = request.outputs[i];
        ::android::nn::wrapper::OperandType operandType =
                model.getOperands()[model.getOutputs()[i]];

        if (!output.hasNoValue) {
            if (output.dimensions.size() > 0) {
                operandType.updateDimensions(::android::nn::toUnsigned(output.dimensions).value());
            }
            auto result = execution->setOutputFromMemory(
                    i, requestMemoryPools->at(output.location.poolIndex).get(),
                    output.location.offset, output.location.length, &operandType.operandType);
            if (result != Result::NO_ERROR) {
                return convertResultToErrorStatus(result);
            }
        } else {
            auto result = execution->setOutput(i, nullptr, 0);
            if (result != Result::NO_ERROR) {
                return convertResultToErrorStatus(result);
            }
        }
    }

    if (measure) {
        execution->setMeasureTiming(true);
    }

    if (deadlineNs > -1) {
        std::chrono::time_point<::android::base::boot_clock> deadlinePoint(
                std::chrono::nanoseconds{deadlineNs});
        const auto currentTime = ::android::base::boot_clock::now();
        const auto timeoutDuration = std::chrono::nanoseconds(deadlinePoint - currentTime);
        if (timeoutDuration <= std::chrono::nanoseconds::zero()) {
            return ErrorStatus::MISSED_DEADLINE_TRANSIENT;
        } else {
            auto result = execution->setTimeout(std::max<uint64_t>(1, timeoutDuration.count()));
            if (result != Result::NO_ERROR) {
                return convertResultToErrorStatus(result);
            }
        }
    }

    if (loopTimeoutDurationNs > 0) {
        execution->setLoopTimeout(loopTimeoutDurationNs);
    }
    return ErrorStatus::NONE;
}

class ShimFencedExecutionCallback : public BnFencedExecutionCallback {
   public:
    ShimFencedExecutionCallback(
            ::android::nn::sl_wrapper::Execution execution, Event e,
            std::vector<std::shared_ptr<::android::nn::sl_wrapper::Memory>> memoryPools,
            bool measureTiming)
        : mMemoryPools(std::move(memoryPools)),
          mExecution(std::move(execution)),
          mEvent(std::move(e)),
          mMeasureTiming(measureTiming) {}

    ndk::ScopedAStatus getExecutionInfo(Timing* timingLaunched, Timing* timingFenced,
                                        ErrorStatus* errorStatus) override {
        auto status = mEvent.wait();
        *errorStatus = convertResultToErrorStatus(status);

        if (mMeasureTiming) {
            uint64_t duration;
            constexpr int64_t int64cap = std::numeric_limits<int64_t>::max();
            // Special value used for "no measurements"
            constexpr uint64_t uint64cap = std::numeric_limits<uint64_t>::max();
            auto result = mExecution.getDuration(Duration::ON_HARDWARE, &duration);
            SLW2SAS_RETURN_IF_ERROR(result);
            timingLaunched->timeOnDeviceNs =
                    (duration == uint64cap)
                            ? -1
                            : (duration > int64cap) ? int64cap : static_cast<int64_t>(duration);

            result = mExecution.getDuration(Duration::IN_DRIVER, &duration);
            SLW2SAS_RETURN_IF_ERROR(result);
            timingLaunched->timeInDriverNs =
                    (duration == uint64cap)
                            ? -1
                            : (duration > int64cap) ? int64cap : static_cast<int64_t>(duration);

            result = mExecution.getDuration(Duration::FENCED_ON_HARDWARE, &duration);
            SLW2SAS_RETURN_IF_ERROR(result);
            timingFenced->timeOnDeviceNs =
                    (duration == uint64cap)
                            ? -1
                            : (duration > int64cap) ? int64cap : static_cast<int64_t>(duration);

            result = mExecution.getDuration(Duration::FENCED_IN_DRIVER, &duration);
            SLW2SAS_RETURN_IF_ERROR(result);
            timingFenced->timeInDriverNs =
                    (duration == uint64cap)
                            ? -1
                            : (duration > int64cap) ? int64cap : static_cast<int64_t>(duration);
        } else {
            timingFenced->timeOnDeviceNs = -1;
            timingFenced->timeInDriverNs = -1;
            timingLaunched->timeOnDeviceNs = -1;
            timingLaunched->timeInDriverNs = -1;
        }

        return ndk::ScopedAStatus::ok();
    }

   private:
    std::vector<std::shared_ptr<::android::nn::sl_wrapper::Memory>> mMemoryPools;
    ::android::nn::sl_wrapper::Execution mExecution;
    ::android::nn::wrapper::Event mEvent;
    bool mMeasureTiming;
};

::ndk::ScopedAStatus ShimPreparedModel::executeFenced(
        const ::aidl::android::hardware::neuralnetworks::Request& request,
        const std::vector<::ndk::ScopedFileDescriptor>& waitFor, bool measureTiming,
        int64_t deadlineNs, int64_t loopTimeoutDurationNs, int64_t durationNs,
        FencedExecutionResult* fencedExecutionResult) {
    if (deadlineNs < -1) {
        LOG(ERROR) << "Invalid deadline value, must be >= -1";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(ErrorStatus::INVALID_ARGUMENT));
    }
    auto execution = ::android::nn::sl_wrapper::Execution(mNnapi.get(), &mCompilation);
    std::vector<std::shared_ptr<::android::nn::sl_wrapper::Memory>> requestMemoryPools;
    auto errorStatus = parseInputs(request, measureTiming, deadlineNs, loopTimeoutDurationNs,
                                   &execution, &requestMemoryPools);
    if (errorStatus != ErrorStatus::NONE) {
        return toAStatus(errorStatus);
    }

    std::vector<const ANeuralNetworksEvent*> deps(waitFor.size());
    auto createResult = Result::NO_ERROR;
    std::transform(waitFor.begin(), waitFor.end(), deps.begin(),
                   [&](const ::ndk::ScopedFileDescriptor& e) {
                       ANeuralNetworksEvent* r = nullptr;
                       if (createResult == Result::NO_ERROR) {
                           createResult = static_cast<Result>(
                                   mNnapi->ANeuralNetworksEvent_createFromSyncFenceFd(e.get(), &r));
                       }
                       return r;
                   });

    const auto guard = ::android::base::make_scope_guard([this, deps] {
        for (auto& dep : deps) {
            if (dep != nullptr) {
                mNnapi->ANeuralNetworksEvent_free(const_cast<ANeuralNetworksEvent*>(dep));
            }
        }
    });

    SLW2SAS_RETURN_IF_ERROR(createResult);

    Event e(mNnapi.get());
    auto result = execution.startComputeWithDependencies(deps, durationNs, &e);
    SLW2SAS_RETURN_IF_ERROR(result);

    int syncFence = -1;
    fencedExecutionResult->syncFence = ndk::ScopedFileDescriptor(
            (e.getSyncFenceFd(&syncFence) == Result::NO_ERROR) ? syncFence : -1);
    fencedExecutionResult->callback = ndk::SharedRefBase::make<ShimFencedExecutionCallback>(
            std::move(execution), std::move(e), std::move(requestMemoryPools), measureTiming);

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ShimPreparedModel::executeSynchronously(
        const Request& request, bool measureTiming, int64_t deadlineNs,
        int64_t loopTimeoutDurationNs,
        ::aidl::android::hardware::neuralnetworks::ExecutionResult* executionResult) {
    if (deadlineNs < -1) {
        LOG(ERROR) << "Invalid deadline value, must be >= -1";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(ErrorStatus::INVALID_ARGUMENT));
    }

    auto execution =
            std::make_unique<::android::nn::sl_wrapper::Execution>(mNnapi.get(), &mCompilation);
    std::vector<std::shared_ptr<::android::nn::sl_wrapper::Memory>> requestMemoryPools;
    auto errorStatus = parseInputs(request, measureTiming, deadlineNs, loopTimeoutDurationNs,
                                   execution.get(), &requestMemoryPools);
    if (errorStatus != ErrorStatus::NONE) {
        return toAStatus(errorStatus);
    }

    auto result = execution->compute();
    errorStatus = convertResultToErrorStatus(result);

    int numOutputs = request.outputs.size();
    std::vector<OutputShape> outputShapes;
    outputShapes.reserve(numOutputs);
    bool sufficientSize = true;
    for (int i = 0; i < numOutputs; ++i) {
        OutputShape outputShape;
        std::vector<uint32_t> outputDims;
        auto result = execution->getOutputOperandDimensions(i, &outputDims);
        if (result == Result::NO_ERROR) {
            outputShape.isSufficient = true;
            outputShape.dimensions.assign(outputDims.begin(), outputDims.end());
        } else if (result == Result::OUTPUT_INSUFFICIENT_SIZE) {
            sufficientSize = false;
            outputShape.isSufficient = false;
            outputShape.dimensions.assign(outputDims.begin(), outputDims.end());
        } else {
            if (errorStatus == ErrorStatus::NONE) {
                errorStatus = ErrorStatus::GENERAL_FAILURE;
            }
        }
        outputShapes.push_back(std::move(outputShape));
    }

    int64_t timeOnDeviceNs = -1;
    int64_t timeInDriverNs = -1;
    if (measureTiming && errorStatus == ErrorStatus::NONE) {
        uint64_t duration;
        constexpr int64_t int64cap = std::numeric_limits<int64_t>::max();
        // Special value used for "no measurements"
        constexpr uint64_t uint64cap = std::numeric_limits<uint64_t>::max();
        auto result = execution->getDuration(Duration::ON_HARDWARE, &duration);
        SLW2SAS_RETURN_IF_ERROR(result);
        timeOnDeviceNs =
                (duration == uint64cap)
                        ? -1
                        : (duration > int64cap) ? int64cap : static_cast<int64_t>(duration);

        result = execution->getDuration(Duration::IN_DRIVER, &duration);
        SLW2SAS_RETURN_IF_ERROR(result);
        timeInDriverNs =
                (duration == uint64cap)
                        ? -1
                        : (duration > int64cap) ? int64cap : static_cast<int64_t>(duration);
    }

    *executionResult =
            ExecutionResult{sufficientSize,
                            std::move(outputShapes),
                            {.timeOnDeviceNs = timeOnDeviceNs, .timeInDriverNs = timeInDriverNs}};
    if (errorStatus == ErrorStatus::NONE || errorStatus == ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        return ndk::ScopedAStatus::ok();
    }
    return toAStatus(errorStatus);
}

// TODO(183397380): make it use ANNBurst object
class ShimBurst : public BnBurst {
   public:
    // Precondition: preparedModel != nullptr
    explicit ShimBurst(std::shared_ptr<ShimPreparedModel> preparedModel);

    ndk::ScopedAStatus executeSynchronously(const Request& request,
                                            const std::vector<int64_t>& memoryIdentifierTokens,
                                            bool measureTiming, int64_t deadlineNs,
                                            int64_t loopTimeoutDurationNs,
                                            ExecutionResult* executionResult) override;
    ndk::ScopedAStatus releaseMemoryResource(int64_t memoryIdentifierToken) override;

   protected:
    std::atomic_flag mExecutionInFlight = ATOMIC_FLAG_INIT;
    const std::shared_ptr<ShimPreparedModel> kPreparedModel;
};

ndk::ScopedAStatus ShimPreparedModel::configureExecutionBurst(std::shared_ptr<IBurst>* burst) {
    std::shared_ptr<ShimPreparedModel> self = this->template ref<ShimPreparedModel>();
    *burst = ndk::SharedRefBase::make<ShimBurst>(std::move(self));
    return ndk::ScopedAStatus::ok();
}

ShimBurst::ShimBurst(std::shared_ptr<ShimPreparedModel> preparedModel)
    : kPreparedModel(std::move(preparedModel)) {
    CHECK(kPreparedModel != nullptr);
}

ndk::ScopedAStatus ShimBurst::executeSynchronously(
        const Request& request, const std::vector<int64_t>& memoryIdentifierTokens,
        bool measureTiming, int64_t deadlineNs, int64_t loopTimeoutDurationNs,
        ExecutionResult* executionResult) {
    if (request.pools.size() != memoryIdentifierTokens.size()) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "request.pools.size() != memoryIdentifierTokens.size()");
    }
    if (!std::all_of(memoryIdentifierTokens.begin(), memoryIdentifierTokens.end(),
                     [](int64_t token) { return token >= -1; })) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT, "Invalid memoryIdentifierTokens");
    }

    // Ensure at most one execution is in flight at a time.
    const bool executionAlreadyInFlight = mExecutionInFlight.test_and_set();
    if (executionAlreadyInFlight) {
        return toAStatus(ErrorStatus::GENERAL_FAILURE,
                         "Burst object supports at most one execution at a time");
    }
    const auto guard = ::android::base::make_scope_guard([this] { mExecutionInFlight.clear(); });

    return kPreparedModel->executeSynchronously(request, measureTiming, deadlineNs,
                                                loopTimeoutDurationNs, executionResult);
}

ndk::ScopedAStatus ShimBurst::releaseMemoryResource(int64_t memoryIdentifierToken) {
    if (memoryIdentifierToken < -1) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT, "Invalid memoryIdentifierToken");
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::neuralnetworks
