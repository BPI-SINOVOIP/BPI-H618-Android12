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

#include "TypeUtils.h"

#include <android-base/logging.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>
#include <vector>

#include "OperandTypes.h"
#include "OperationTypes.h"
#include "Result.h"
#include "SharedMemory.h"
#include "Types.h"

namespace android::nn {
namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type object) {
    return static_cast<std::underlying_type_t<Type>>(object);
}

template <typename Type>
std::ostream& operator<<(std::ostream& os, const std::vector<Type>& vec) {
    constexpr size_t kMaxVectorPrint = 20;
    os << "[";
    size_t count = 0;
    for (const auto& element : vec) {
        if (count > 0) {
            os << ", ";
        }
        os << element;
        count++;
        if (count >= kMaxVectorPrint) {
            return os << "...]";
        }
    }
    return os << "]";
}

}  // namespace

bool isExtension(OperandType type) {
    return getExtensionPrefix(underlyingType(type)) != 0;
}

bool isExtension(OperationType type) {
    return getExtensionPrefix(underlyingType(type)) != 0;
}

bool isNonExtensionScalar(OperandType operandType) {
    CHECK(!isExtension(operandType));
    switch (operandType) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::BOOL:
        case OperandType::FLOAT16:
        case OperandType::SUBGRAPH:
        case OperandType::OEM:
            return true;
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_BOOL8:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT16_ASYMM:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case OperandType::TENSOR_OEM_BYTE:
            return false;
    }
    return false;
}

size_t getNonExtensionSize(OperandType operandType) {
    CHECK(!isExtension(operandType));
    switch (operandType) {
        case OperandType::SUBGRAPH:
        case OperandType::OEM:
            return 0;
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::BOOL:
        case OperandType::TENSOR_BOOL8:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case OperandType::TENSOR_OEM_BYTE:
            return 1;
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT16_ASYMM:
            return 2;
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
            return 4;
    }
    return 0;
}

std::optional<size_t> getNonExtensionSize(OperandType operandType, const Dimensions& dimensions) {
    CHECK(!isExtension(operandType)) << "Size of extension operand data is unknown";
    size_t size = getNonExtensionSize(operandType);
    if (isNonExtensionScalar(operandType)) {
        return size;
    } else if (dimensions.empty()) {
        return 0;
    }
    for (Dimension dimension : dimensions) {
        if (dimension != 0 && size > std::numeric_limits<size_t>::max() / dimension) {
            return std::nullopt;
        }
        size *= dimension;
    }
    return size;
}

std::optional<size_t> getNonExtensionSize(const Operand& operand) {
    return getNonExtensionSize(operand.type, operand.dimensions);
}

size_t getOffsetFromInts(int lower, int higher) {
    const int32_t lowBits = static_cast<int32_t>(lower);
    const int32_t highBits = static_cast<int32_t>(higher);
    const uint32_t lowOffsetBits = *reinterpret_cast<const uint32_t*>(&lowBits);
    const uint32_t highOffsetBits = *reinterpret_cast<const uint32_t*>(&highBits);
    const uint64_t offset = lowOffsetBits | (static_cast<uint64_t>(highOffsetBits) << 32);
    return offset;
}

std::pair<int32_t, int32_t> getIntsFromOffset(size_t offset) {
    const uint64_t bits = static_cast<uint64_t>(offset);
    const uint32_t lowBits = static_cast<uint32_t>(bits & 0xffffffff);
    const uint32_t highBits = static_cast<uint32_t>(bits >> 32);
    const int32_t lowOffsetBits = *reinterpret_cast<const int32_t*>(&lowBits);
    const int32_t highOffsetBits = *reinterpret_cast<const int32_t*>(&highBits);
    return std::make_pair(lowOffsetBits, highOffsetBits);
}

Result<std::vector<uint32_t>> countNumberOfConsumers(size_t numberOfOperands,
                                                     const std::vector<nn::Operation>& operations) {
    std::vector<uint32_t> numberOfConsumers(numberOfOperands, 0);
    for (const auto& operation : operations) {
        for (uint32_t operandIndex : operation.inputs) {
            if (operandIndex >= numberOfConsumers.size()) {
                return NN_ERROR()
                       << "countNumberOfConsumers: tried to access out-of-bounds operand ("
                       << operandIndex << " vs " << numberOfConsumers.size() << ")";
            }
            numberOfConsumers[operandIndex]++;
        }
    }
    return numberOfConsumers;
}

Result<Dimensions> combineDimensions(const Dimensions& lhs, const Dimensions& rhs) {
    if (rhs.empty()) return lhs;
    if (lhs.empty()) return rhs;
    if (lhs.size() != rhs.size()) {
        std::ostringstream os;
        os << "Incompatible ranks: " << lhs << " and " << rhs;
        return NN_ERROR() << os.str();
    }
    Dimensions combined = lhs;
    for (size_t i = 0; i < lhs.size(); i++) {
        if (lhs[i] == 0) {
            combined[i] = rhs[i];
        } else if (rhs[i] != 0 && lhs[i] != rhs[i]) {
            std::ostringstream os;
            os << "Incompatible dimensions: " << lhs << " and " << rhs;
            return NN_ERROR() << os.str();
        }
    }
    return combined;
}

std::pair<size_t, std::vector<size_t>> getMemorySizes(const Model& model) {
    const size_t operandValuesSize = model.operandValues.size();

    std::vector<size_t> poolSizes;
    poolSizes.reserve(model.pools.size());
    std::transform(model.pools.begin(), model.pools.end(), std::back_inserter(poolSizes),
                   [](const SharedMemory& memory) { return getSize(memory); });

    return std::make_pair(operandValuesSize, std::move(poolSizes));
}

size_t roundUp(size_t size, size_t multiple) {
    CHECK(multiple != 0);
    CHECK((multiple & (multiple - 1)) == 0) << multiple << " is not a power of two";
    return (size + (multiple - 1)) & ~(multiple - 1);
}

size_t getAlignmentForLength(size_t length) {
    if (length < 2) {
        return 1;  // No alignment necessary
    } else if (length < 4) {
        return 2;  // Align on 2-byte boundary
    } else {
        return 4;  // Align on 4-byte boundary
    }
}

std::ostream& operator<<(std::ostream& os, const DeviceStatus& deviceStatus) {
    switch (deviceStatus) {
        case DeviceStatus::AVAILABLE:
            return os << "AVAILABLE";
        case DeviceStatus::BUSY:
            return os << "BUSY";
        case DeviceStatus::OFFLINE:
            return os << "OFFLINE";
        case DeviceStatus::UNKNOWN:
            return os << "UNKNOWN";
    }
    return os << "DeviceStatus{" << underlyingType(deviceStatus) << "}";
}

std::ostream& operator<<(std::ostream& os, const ExecutionPreference& executionPreference) {
    switch (executionPreference) {
        case ExecutionPreference::LOW_POWER:
            return os << "LOW_POWER";
        case ExecutionPreference::FAST_SINGLE_ANSWER:
            return os << "FAST_SINGLE_ANSWER";
        case ExecutionPreference::SUSTAINED_SPEED:
            return os << "SUSTAINED_SPEED";
    }
    return os << "ExecutionPreference{" << underlyingType(executionPreference) << "}";
}

std::ostream& operator<<(std::ostream& os, const DeviceType& deviceType) {
    switch (deviceType) {
        case DeviceType::UNKNOWN:
            return os << "UNKNOWN";
        case DeviceType::OTHER:
            return os << "OTHER";
        case DeviceType::CPU:
            return os << "CPU";
        case DeviceType::GPU:
            return os << "GPU";
        case DeviceType::ACCELERATOR:
            return os << "ACCELERATOR";
    }
    return os << "DeviceType{" << underlyingType(deviceType) << "}";
}

std::ostream& operator<<(std::ostream& os, const MeasureTiming& measureTiming) {
    switch (measureTiming) {
        case MeasureTiming::NO:
            return os << "NO";
        case MeasureTiming::YES:
            return os << "YES";
    }
    return os << "MeasureTiming{" << underlyingType(measureTiming) << "}";
}

std::ostream& operator<<(std::ostream& os, const OperandType& operandType) {
    switch (operandType) {
        case OperandType::FLOAT32:
            return os << "FLOAT32";
        case OperandType::INT32:
            return os << "INT32";
        case OperandType::UINT32:
            return os << "UINT32";
        case OperandType::TENSOR_FLOAT32:
            return os << "TENSOR_FLOAT32";
        case OperandType::TENSOR_INT32:
            return os << "TENSOR_INT32";
        case OperandType::TENSOR_QUANT8_ASYMM:
            return os << "TENSOR_QUANT8_ASYMM";
        case OperandType::BOOL:
            return os << "BOOL";
        case OperandType::TENSOR_QUANT16_SYMM:
            return os << "TENSOR_QUANT16_SYMM";
        case OperandType::TENSOR_FLOAT16:
            return os << "TENSOR_FLOAT16";
        case OperandType::TENSOR_BOOL8:
            return os << "TENSOR_BOOL8";
        case OperandType::FLOAT16:
            return os << "FLOAT16";
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
            return os << "TENSOR_QUANT8_SYMM_PER_CHANNEL";
        case OperandType::TENSOR_QUANT16_ASYMM:
            return os << "TENSOR_QUANT16_ASYMM";
        case OperandType::TENSOR_QUANT8_SYMM:
            return os << "TENSOR_QUANT8_SYMM";
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return os << "TENSOR_QUANT8_ASYMM_SIGNED";
        case OperandType::SUBGRAPH:
            return os << "SUBGRAPH";
        case OperandType::OEM:
            return os << "OEM";
        case OperandType::TENSOR_OEM_BYTE:
            return os << "TENSOR_OEM_BYTE";
    }
    if (isExtension(operandType)) {
        return os << "Extension OperandType " << underlyingType(operandType);
    }
    return os << "OperandType{" << underlyingType(operandType) << "}";
}

std::ostream& operator<<(std::ostream& os, const Operand::LifeTime& lifetime) {
    switch (lifetime) {
        case Operand::LifeTime::TEMPORARY_VARIABLE:
            return os << "TEMPORARY_VARIABLE";
        case Operand::LifeTime::SUBGRAPH_INPUT:
            return os << "SUBGRAPH_INPUT";
        case Operand::LifeTime::SUBGRAPH_OUTPUT:
            return os << "SUBGRAPH_OUTPUT";
        case Operand::LifeTime::CONSTANT_COPY:
            return os << "CONSTANT_COPY";
        case Operand::LifeTime::CONSTANT_REFERENCE:
            return os << "CONSTANT_REFERENCE";
        case Operand::LifeTime::NO_VALUE:
            return os << "NO_VALUE";
        case Operand::LifeTime::SUBGRAPH:
            return os << "SUBGRAPH";
        case Operand::LifeTime::POINTER:
            return os << "POINTER";
    }
    return os << "Operand::LifeTime{" << underlyingType(lifetime) << "}";
}

std::ostream& operator<<(std::ostream& os, const OperationType& operationType) {
    switch (operationType) {
        case OperationType::ADD:
            return os << "ADD";
        case OperationType::AVERAGE_POOL_2D:
            return os << "AVERAGE_POOL_2D";
        case OperationType::CONCATENATION:
            return os << "CONCATENATION";
        case OperationType::CONV_2D:
            return os << "CONV_2D";
        case OperationType::DEPTHWISE_CONV_2D:
            return os << "DEPTHWISE_CONV_2D";
        case OperationType::DEPTH_TO_SPACE:
            return os << "DEPTH_TO_SPACE";
        case OperationType::DEQUANTIZE:
            return os << "DEQUANTIZE";
        case OperationType::EMBEDDING_LOOKUP:
            return os << "EMBEDDING_LOOKUP";
        case OperationType::FLOOR:
            return os << "FLOOR";
        case OperationType::FULLY_CONNECTED:
            return os << "FULLY_CONNECTED";
        case OperationType::HASHTABLE_LOOKUP:
            return os << "HASHTABLE_LOOKUP";
        case OperationType::L2_NORMALIZATION:
            return os << "L2_NORMALIZATION";
        case OperationType::L2_POOL_2D:
            return os << "L2_POOL_2D";
        case OperationType::LOCAL_RESPONSE_NORMALIZATION:
            return os << "LOCAL_RESPONSE_NORMALIZATION";
        case OperationType::LOGISTIC:
            return os << "LOGISTIC";
        case OperationType::LSH_PROJECTION:
            return os << "LSH_PROJECTION";
        case OperationType::LSTM:
            return os << "LSTM";
        case OperationType::MAX_POOL_2D:
            return os << "MAX_POOL_2D";
        case OperationType::MUL:
            return os << "MUL";
        case OperationType::RELU:
            return os << "RELU";
        case OperationType::RELU1:
            return os << "RELU1";
        case OperationType::RELU6:
            return os << "RELU6";
        case OperationType::RESHAPE:
            return os << "RESHAPE";
        case OperationType::RESIZE_BILINEAR:
            return os << "RESIZE_BILINEAR";
        case OperationType::RNN:
            return os << "RNN";
        case OperationType::SOFTMAX:
            return os << "SOFTMAX";
        case OperationType::SPACE_TO_DEPTH:
            return os << "SPACE_TO_DEPTH";
        case OperationType::SVDF:
            return os << "SVDF";
        case OperationType::TANH:
            return os << "TANH";
        case OperationType::BATCH_TO_SPACE_ND:
            return os << "BATCH_TO_SPACE_ND";
        case OperationType::DIV:
            return os << "DIV";
        case OperationType::MEAN:
            return os << "MEAN";
        case OperationType::PAD:
            return os << "PAD";
        case OperationType::SPACE_TO_BATCH_ND:
            return os << "SPACE_TO_BATCH_ND";
        case OperationType::SQUEEZE:
            return os << "SQUEEZE";
        case OperationType::STRIDED_SLICE:
            return os << "STRIDED_SLICE";
        case OperationType::SUB:
            return os << "SUB";
        case OperationType::TRANSPOSE:
            return os << "TRANSPOSE";
        case OperationType::ABS:
            return os << "ABS";
        case OperationType::ARGMAX:
            return os << "ARGMAX";
        case OperationType::ARGMIN:
            return os << "ARGMIN";
        case OperationType::AXIS_ALIGNED_BBOX_TRANSFORM:
            return os << "AXIS_ALIGNED_BBOX_TRANSFORM";
        case OperationType::BIDIRECTIONAL_SEQUENCE_LSTM:
            return os << "BIDIRECTIONAL_SEQUENCE_LSTM";
        case OperationType::BIDIRECTIONAL_SEQUENCE_RNN:
            return os << "BIDIRECTIONAL_SEQUENCE_RNN";
        case OperationType::BOX_WITH_NMS_LIMIT:
            return os << "BOX_WITH_NMS_LIMIT";
        case OperationType::CAST:
            return os << "CAST";
        case OperationType::CHANNEL_SHUFFLE:
            return os << "CHANNEL_SHUFFLE";
        case OperationType::DETECTION_POSTPROCESSING:
            return os << "DETECTION_POSTPROCESSING";
        case OperationType::EQUAL:
            return os << "EQUAL";
        case OperationType::EXP:
            return os << "EXP";
        case OperationType::EXPAND_DIMS:
            return os << "EXPAND_DIMS";
        case OperationType::GATHER:
            return os << "GATHER";
        case OperationType::GENERATE_PROPOSALS:
            return os << "GENERATE_PROPOSALS";
        case OperationType::GREATER:
            return os << "GREATER";
        case OperationType::GREATER_EQUAL:
            return os << "GREATER_EQUAL";
        case OperationType::GROUPED_CONV_2D:
            return os << "GROUPED_CONV_2D";
        case OperationType::HEATMAP_MAX_KEYPOINT:
            return os << "HEATMAP_MAX_KEYPOINT";
        case OperationType::INSTANCE_NORMALIZATION:
            return os << "INSTANCE_NORMALIZATION";
        case OperationType::LESS:
            return os << "LESS";
        case OperationType::LESS_EQUAL:
            return os << "LESS_EQUAL";
        case OperationType::LOG:
            return os << "LOG";
        case OperationType::LOGICAL_AND:
            return os << "LOGICAL_AND";
        case OperationType::LOGICAL_NOT:
            return os << "LOGICAL_NOT";
        case OperationType::LOGICAL_OR:
            return os << "LOGICAL_OR";
        case OperationType::LOG_SOFTMAX:
            return os << "LOG_SOFTMAX";
        case OperationType::MAXIMUM:
            return os << "MAXIMUM";
        case OperationType::MINIMUM:
            return os << "MINIMUM";
        case OperationType::NEG:
            return os << "NEG";
        case OperationType::NOT_EQUAL:
            return os << "NOT_EQUAL";
        case OperationType::PAD_V2:
            return os << "PAD_V2";
        case OperationType::POW:
            return os << "POW";
        case OperationType::PRELU:
            return os << "PRELU";
        case OperationType::QUANTIZE:
            return os << "QUANTIZE";
        case OperationType::QUANTIZED_16BIT_LSTM:
            return os << "QUANTIZED_16BIT_LSTM";
        case OperationType::RANDOM_MULTINOMIAL:
            return os << "RANDOM_MULTINOMIAL";
        case OperationType::REDUCE_ALL:
            return os << "REDUCE_ALL";
        case OperationType::REDUCE_ANY:
            return os << "REDUCE_ANY";
        case OperationType::REDUCE_MAX:
            return os << "REDUCE_MAX";
        case OperationType::REDUCE_MIN:
            return os << "REDUCE_MIN";
        case OperationType::REDUCE_PROD:
            return os << "REDUCE_PROD";
        case OperationType::REDUCE_SUM:
            return os << "REDUCE_SUM";
        case OperationType::ROI_ALIGN:
            return os << "ROI_ALIGN";
        case OperationType::ROI_POOLING:
            return os << "ROI_POOLING";
        case OperationType::RSQRT:
            return os << "RSQRT";
        case OperationType::SELECT:
            return os << "SELECT";
        case OperationType::SIN:
            return os << "SIN";
        case OperationType::SLICE:
            return os << "SLICE";
        case OperationType::SPLIT:
            return os << "SPLIT";
        case OperationType::SQRT:
            return os << "SQRT";
        case OperationType::TILE:
            return os << "TILE";
        case OperationType::TOPK_V2:
            return os << "TOPK_V2";
        case OperationType::TRANSPOSE_CONV_2D:
            return os << "TRANSPOSE_CONV_2D";
        case OperationType::UNIDIRECTIONAL_SEQUENCE_LSTM:
            return os << "UNIDIRECTIONAL_SEQUENCE_LSTM";
        case OperationType::UNIDIRECTIONAL_SEQUENCE_RNN:
            return os << "UNIDIRECTIONAL_SEQUENCE_RNN";
        case OperationType::RESIZE_NEAREST_NEIGHBOR:
            return os << "RESIZE_NEAREST_NEIGHBOR";
        case OperationType::QUANTIZED_LSTM:
            return os << "QUANTIZED_LSTM";
        case OperationType::IF:
            return os << "IF";
        case OperationType::WHILE:
            return os << "WHILE";
        case OperationType::ELU:
            return os << "ELU";
        case OperationType::HARD_SWISH:
            return os << "HARD_SWISH";
        case OperationType::FILL:
            return os << "FILL";
        case OperationType::RANK:
            return os << "RANK";
        case OperationType::OEM_OPERATION:
            return os << "OEM_OPERATION";
    }
    if (isExtension(operationType)) {
        return os << "Extension OperationType " << underlyingType(operationType);
    }
    return os << "OperationType{" << underlyingType(operationType) << "}";
}

std::ostream& operator<<(std::ostream& os, const Request::Argument::LifeTime& lifetime) {
    switch (lifetime) {
        case Request::Argument::LifeTime::POOL:
            return os << "POOL";
        case Request::Argument::LifeTime::NO_VALUE:
            return os << "NO_VALUE";
        case Request::Argument::LifeTime::POINTER:
            return os << "POINTER";
    }
    return os << "Request::Argument::LifeTime{" << underlyingType(lifetime) << "}";
}

std::ostream& operator<<(std::ostream& os, const Priority& priority) {
    switch (priority) {
        case Priority::LOW:
            return os << "LOW";
        case Priority::MEDIUM:
            return os << "MEDIUM";
        case Priority::HIGH:
            return os << "HIGH";
    }
    return os << "Priority{" << underlyingType(priority) << "}";
}

std::ostream& operator<<(std::ostream& os, const ErrorStatus& errorStatus) {
    switch (errorStatus) {
        case ErrorStatus::NONE:
            return os << "NONE";
        case ErrorStatus::DEVICE_UNAVAILABLE:
            return os << "DEVICE_UNAVAILABLE";
        case ErrorStatus::GENERAL_FAILURE:
            return os << "GENERAL_FAILURE";
        case ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
            return os << "OUTPUT_INSUFFICIENT_SIZE";
        case ErrorStatus::INVALID_ARGUMENT:
            return os << "INVALID_ARGUMENT";
        case ErrorStatus::MISSED_DEADLINE_TRANSIENT:
            return os << "MISSED_DEADLINE_TRANSIENT";
        case ErrorStatus::MISSED_DEADLINE_PERSISTENT:
            return os << "MISSED_DEADLINE_PERSISTENT";
        case ErrorStatus::RESOURCE_EXHAUSTED_TRANSIENT:
            return os << "RESOURCE_EXHAUSTED_TRANSIENT";
        case ErrorStatus::RESOURCE_EXHAUSTED_PERSISTENT:
            return os << "RESOURCE_EXHAUSTED_PERSISTENT";
        case ErrorStatus::DEAD_OBJECT:
            return os << "DEAD_OBJECT";
    }
    return os << "ErrorStatus{" << underlyingType(errorStatus) << "}";
}

std::ostream& operator<<(std::ostream& os, const FusedActivationFunc& activation) {
    switch (activation) {
        case FusedActivationFunc::NONE:
            return os << "NONE";
        case FusedActivationFunc::RELU:
            return os << "RELU";
        case FusedActivationFunc::RELU1:
            return os << "RELU1";
        case FusedActivationFunc::RELU6:
            return os << "RELU6";
    }
    return os << "FusedActivationFunc{" << underlyingType(activation) << "}";
}

std::ostream& operator<<(std::ostream& os, const OutputShape& outputShape) {
    return os << "OutputShape{.dimensions=" << outputShape.dimensions
              << ", .isSufficient=" << (outputShape.isSufficient ? "true" : "false") << "}";
}

std::ostream& operator<<(std::ostream& os, const Timing& timing) {
    return os << "Timing{.timeOnDevice=" << timing.timeOnDevice
              << ", .timeInDriver=" << timing.timeInDriver << "}";
}

std::ostream& operator<<(std::ostream& os, const Capabilities::PerformanceInfo& performanceInfo) {
    return os << "Capabilities::PerformanceInfo{.execTime=" << performanceInfo.execTime
              << ", .powerUsage=" << performanceInfo.powerUsage << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const Capabilities::OperandPerformance& operandPerformance) {
    return os << "Capabilities::OperandPerformance{.type=" << operandPerformance.type
              << ", .info=" << operandPerformance.info << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const Capabilities::OperandPerformanceTable& operandPerformances) {
    return os << operandPerformances.asVector();
}

std::ostream& operator<<(std::ostream& os, const Capabilities& capabilities) {
    return os << "Capabilities{.relaxedFloat32toFloat16PerformanceScalar="
              << capabilities.relaxedFloat32toFloat16PerformanceScalar
              << ", .relaxedFloat32toFloat16PerformanceTensor="
              << capabilities.relaxedFloat32toFloat16PerformanceTensor
              << ", .operandPerformance=" << capabilities.operandPerformance
              << ", .ifPerformance=" << capabilities.ifPerformance
              << ", .whilePerformance=" << capabilities.whilePerformance << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const Extension::OperandTypeInformation& operandTypeInformation) {
    return os << "Extension::OperandTypeInformation{.type=" << operandTypeInformation.type
              << ", .isTensor=" << (operandTypeInformation.isTensor ? "true" : "false")
              << ", .byteSize=" << operandTypeInformation.byteSize << "}";
}

std::ostream& operator<<(std::ostream& os, const Extension& extension) {
    return os << "Extension{.name=" << extension.name
              << ", .operandTypes=" << extension.operandTypes << "}";
}

std::ostream& operator<<(std::ostream& os, const DataLocation& location) {
    const auto printPointer = [&os](const std::variant<const void*, void*>& pointer) {
        os << (std::holds_alternative<const void*>(pointer) ? "<constant " : "<mutable ");
        os << std::visit(
                [](const auto* ptr) {
                    return ptr == nullptr ? "null pointer>" : "non-null pointer>";
                },
                pointer);
    };
    os << "DataLocation{.pointer=";
    printPointer(location.pointer);
    return os << ", .poolIndex=" << location.poolIndex << ", .offset=" << location.offset
              << ", .length=" << location.length << ", .padding=" << location.padding << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams) {
    return os << "Operand::SymmPerChannelQuantParams{.scales=" << symmPerChannelQuantParams.scales
              << ", .channelDim=" << symmPerChannelQuantParams.channelDim << "}";
}

std::ostream& operator<<(std::ostream& os, const Operand::ExtraParams& extraParams) {
    os << "Operand::ExtraParams{";
    if (std::holds_alternative<Operand::NoParams>(extraParams)) {
        os << "<no params>";
    } else if (std::holds_alternative<Operand::SymmPerChannelQuantParams>(extraParams)) {
        os << std::get<Operand::SymmPerChannelQuantParams>(extraParams);
    } else if (std::holds_alternative<Operand::ExtensionParams>(extraParams)) {
        os << std::get<Operand::ExtensionParams>(extraParams);
    }
    return os << "}";
}

std::ostream& operator<<(std::ostream& os, const Operand& operand) {
    return os << "Operand{.type=" << operand.type << ", .dimensions=" << operand.dimensions
              << ", .scale=" << operand.scale << ", .zeroPoint=" << operand.zeroPoint
              << ", lifetime=" << operand.lifetime << ", .location=" << operand.location
              << ", .extraParams=" << operand.extraParams << "}";
}

std::ostream& operator<<(std::ostream& os, const Operation& operation) {
    return os << "Operation{.type=" << operation.type << ", .inputs=" << operation.inputs
              << ", .outputs=" << operation.outputs << "}";
}

static std::ostream& operator<<(std::ostream& os, const Handle& handle) {
    return os << "<handle with " << handle.fds.size() << " fds and " << handle.ints.size()
              << " ints>";
}

std::ostream& operator<<(std::ostream& os, const SharedHandle& handle) {
    if (handle == nullptr) {
        return os << "<empty handle>";
    }
    return os << *handle;
}

static std::ostream& operator<<(std::ostream& os, const Memory::Ashmem& memory) {
    return os << "Ashmem{.fd=" << (memory.fd.ok() ? "<valid fd>" : "<invalid fd>")
              << ", .size=" << memory.size << "}";
}

static std::ostream& operator<<(std::ostream& os, const Memory::Fd& memory) {
    return os << "Fd{.size=" << memory.size << ", .prot=" << memory.prot
              << ", .fd=" << (memory.fd.ok() ? "<valid fd>" : "<invalid fd>")
              << ", .offset=" << memory.offset << "}";
}

static std::ostream& operator<<(std::ostream& os, const Memory::HardwareBuffer& memory) {
    if (memory.handle.get() == nullptr) {
        return os << "<empty HardwareBuffer::Handle>";
    }
    return os << (isAhwbBlob(memory) ? "<AHardwareBuffer blob>" : "<non-blob AHardwareBuffer>");
}

static std::ostream& operator<<(std::ostream& os, const Memory::Unknown& memory) {
    return os << "Unknown{.handle=" << memory.handle << ", .size=" << memory.size
              << ", .name=" << memory.name << "}";
}

std::ostream& operator<<(std::ostream& os, const Memory& memory) {
    os << "Memory{.handle=";
    std::visit([&os](const auto& x) { os << x; }, memory.handle);
    return os << "}";
}

std::ostream& operator<<(std::ostream& os, const SharedMemory& memory) {
    if (memory == nullptr) {
        return os << "<empty memory>";
    }
    return os << *memory;
}

std::ostream& operator<<(std::ostream& os, const MemoryPreference& memoryPreference) {
    return os << "MemoryPreference{.alignment=" << memoryPreference.alignment
              << ", .padding=" << memoryPreference.padding << "}";
}

std::ostream& operator<<(std::ostream& os, const Model::Subgraph& subgraph) {
    std::vector<Operand> operands;
    std::vector<Operation> operations;
    std::vector<uint32_t> inputIndexes;
    std::vector<uint32_t> outputIndexes;
    return os << "Model::Subgraph{.operands=" << subgraph.operands
              << ", .operations=" << subgraph.operations
              << ", .inputIndexes=" << subgraph.inputIndexes
              << ", .outputIndexes=" << subgraph.outputIndexes << "}";
}

std::ostream& operator<<(std::ostream& os, const Model::OperandValues& operandValues) {
    return os << "Model::OperandValues{<" << operandValues.size() << "bytes>}";
}

std::ostream& operator<<(std::ostream& os,
                         const Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return os << "Model::ExtensionNameAndPrefix{.name=" << extensionNameAndPrefix.name
              << ", .prefix=" << extensionNameAndPrefix.prefix << "}";
}

std::ostream& operator<<(std::ostream& os, const Model& model) {
    return os << "Model{.main=" << model.main << ", .referenced=" << model.referenced
              << ", .operandValues=" << model.operandValues << ", .pools=" << model.pools
              << ", .relaxComputationFloat32toFloat16="
              << (model.relaxComputationFloat32toFloat16 ? "true" : "false")
              << ", extensionNameToPrefix=" << model.extensionNameToPrefix << "}";
}

std::ostream& operator<<(std::ostream& os, const BufferDesc& bufferDesc) {
    return os << "BufferDesc{.dimensions=" << bufferDesc.dimensions << "}";
}

std::ostream& operator<<(std::ostream& os, const BufferRole& bufferRole) {
    return os << "BufferRole{.modelIndex=" << bufferRole.modelIndex
              << ", .ioIndex=" << bufferRole.ioIndex << ", .probability=" << bufferRole.probability
              << "}";
}

std::ostream& operator<<(std::ostream& os, const Request::Argument& requestArgument) {
    return os << "Request::Argument{.lifetime=" << requestArgument.lifetime
              << ", .location=" << requestArgument.location
              << ", .dimensions=" << requestArgument.dimensions << "}";
}

std::ostream& operator<<(std::ostream& os, const Request::MemoryPool& memoryPool) {
    os << "Request::MemoryPool{";
    if (std::holds_alternative<SharedMemory>(memoryPool)) {
        os << std::get<SharedMemory>(memoryPool);
    } else if (std::holds_alternative<Request::MemoryDomainToken>(memoryPool)) {
        const auto& token = std::get<Request::MemoryDomainToken>(memoryPool);
        if (token == Request::MemoryDomainToken{}) {
            os << "<invalid MemoryDomainToken>";
        } else {
            os << "MemoryDomainToken=" << underlyingType(token);
        }
    } else if (std::holds_alternative<SharedBuffer>(memoryPool)) {
        const auto& buffer = std::get<SharedBuffer>(memoryPool);
        os << (buffer != nullptr ? "<non-null IBuffer>" : "<null IBuffer>");
    }
    return os << "}";
}

std::ostream& operator<<(std::ostream& os, const Request& request) {
    return os << "Request{.inputs=" << request.inputs << ", .outputs=" << request.outputs
              << ", .pools=" << request.pools << "}";
}

std::ostream& operator<<(std::ostream& os, const SyncFence::FenceState& fenceState) {
    switch (fenceState) {
        case SyncFence::FenceState::ACTIVE:
            return os << "ACTIVE";
        case SyncFence::FenceState::SIGNALED:
            return os << "SIGNALED";
        case SyncFence::FenceState::ERROR:
            return os << "ERROR";
        case SyncFence::FenceState::UNKNOWN:
            return os << "UNKNOWN";
    }
    return os << "SyncFence::FenceState{" << underlyingType(fenceState) << "}";
}

std::ostream& operator<<(std::ostream& os, const TimePoint& timePoint) {
    return os << timePoint.time_since_epoch() << " since epoch";
}

std::ostream& operator<<(std::ostream& os, const OptionalTimePoint& optionalTimePoint) {
    if (!optionalTimePoint.has_value()) {
        return os << "<no time point>";
    }
    return os << optionalTimePoint.value();
}

std::ostream& operator<<(std::ostream& os, const Duration& timeoutDuration) {
    return os << timeoutDuration.count() << "ns";
}

std::ostream& operator<<(std::ostream& os, const OptionalDuration& optionalTimeoutDuration) {
    if (!optionalTimeoutDuration.has_value()) {
        return os << "<no duration>";
    }
    return os << optionalTimeoutDuration.value();
}

std::ostream& operator<<(std::ostream& os, const Version& version) {
    switch (version) {
        case Version::ANDROID_OC_MR1:
            return os << "ANDROID_OC_MR1";
        case Version::ANDROID_P:
            return os << "ANDROID_P";
        case Version::ANDROID_Q:
            return os << "ANDROID_Q";
        case Version::ANDROID_R:
            return os << "ANDROID_R";
        case Version::ANDROID_S:
            return os << "ANDROID_S";
        case Version::CURRENT_RUNTIME:
            return os << "CURRENT_RUNTIME";
    }
    return os << "Version{" << underlyingType(version) << "}";
}

std::ostream& operator<<(std::ostream& os, const HalVersion& halVersion) {
    switch (halVersion) {
        case HalVersion::UNKNOWN:
            return os << "UNKNOWN HAL version";
        case HalVersion::V1_0:
            return os << "HAL version 1.0";
        case HalVersion::V1_1:
            return os << "HAL version 1.1";
        case HalVersion::V1_2:
            return os << "HAL version 1.2";
        case HalVersion::V1_3:
            return os << "HAL version 1.3";
        case HalVersion::AIDL_UNSTABLE:
            return os << "HAL uses unstable AIDL";
    }
    return os << "HalVersion{" << underlyingType(halVersion) << "}";
}

bool operator==(const Timing& a, const Timing& b) {
    return a.timeOnDevice == b.timeOnDevice && a.timeInDriver == b.timeInDriver;
}
bool operator!=(const Timing& a, const Timing& b) {
    return !(a == b);
}

bool operator==(const Capabilities::PerformanceInfo& a, const Capabilities::PerformanceInfo& b) {
    return a.execTime == b.execTime && a.powerUsage == b.powerUsage;
}
bool operator!=(const Capabilities::PerformanceInfo& a, const Capabilities::PerformanceInfo& b) {
    return !(a == b);
}

bool operator==(const Capabilities::OperandPerformance& a,
                const Capabilities::OperandPerformance& b) {
    return a.type == b.type && a.info == b.info;
}
bool operator!=(const Capabilities::OperandPerformance& a,
                const Capabilities::OperandPerformance& b) {
    return !(a == b);
}

bool operator==(const Capabilities& a, const Capabilities& b) {
    return a.relaxedFloat32toFloat16PerformanceScalar ==
                   b.relaxedFloat32toFloat16PerformanceScalar &&
           a.relaxedFloat32toFloat16PerformanceTensor ==
                   b.relaxedFloat32toFloat16PerformanceTensor &&
           a.operandPerformance.asVector() == b.operandPerformance.asVector() &&
           a.ifPerformance == b.ifPerformance && a.whilePerformance == b.whilePerformance;
}
bool operator!=(const Capabilities& a, const Capabilities& b) {
    return !(a == b);
}

bool operator==(const Extension::OperandTypeInformation& a,
                const Extension::OperandTypeInformation& b) {
    return a.type == b.type && a.isTensor == b.isTensor && a.byteSize == b.byteSize;
}
bool operator!=(const Extension::OperandTypeInformation& a,
                const Extension::OperandTypeInformation& b) {
    return !(a == b);
}

bool operator==(const Extension& a, const Extension& b) {
    return a.name == b.name && a.operandTypes == b.operandTypes;
}
bool operator!=(const Extension& a, const Extension& b) {
    return !(a == b);
}

bool operator==(const MemoryPreference& a, const MemoryPreference& b) {
    return a.alignment == b.alignment && a.padding == b.padding;
}
bool operator!=(const MemoryPreference& a, const MemoryPreference& b) {
    return !(a == b);
}

bool operator==(const Operand::SymmPerChannelQuantParams& a,
                const Operand::SymmPerChannelQuantParams& b) {
    return a.scales == b.scales && a.channelDim == b.channelDim;
}
bool operator!=(const Operand::SymmPerChannelQuantParams& a,
                const Operand::SymmPerChannelQuantParams& b) {
    return !(a == b);
}

static bool operator==(const DataLocation& a, const DataLocation& b) {
    constexpr auto toTuple = [](const DataLocation& location) {
        return std::tie(location.pointer, location.poolIndex, location.offset, location.length,
                        location.padding);
    };
    return toTuple(a) == toTuple(b);
}

bool operator==(const Operand& a, const Operand& b) {
    constexpr auto toTuple = [](const Operand& operand) {
        return std::tie(operand.type, operand.dimensions, operand.scale, operand.zeroPoint,
                        operand.lifetime, operand.location, operand.extraParams);
    };
    return toTuple(a) == toTuple(b);
}
bool operator!=(const Operand& a, const Operand& b) {
    return !(a == b);
}

bool operator==(const Operation& a, const Operation& b) {
    constexpr auto toTuple = [](const Operation& operation) {
        return std::tie(operation.type, operation.inputs, operation.outputs);
    };
    return toTuple(a) == toTuple(b);
}
bool operator!=(const Operation& a, const Operation& b) {
    return !(a == b);
}

}  // namespace android::nn
