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

#include "Validation.h"

#include <android-base/logging.h>
#include <android-base/mapped_file.h>

#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "ControlFlow.h"
#include "OperandTypes.h"
#include "OperationResolver.h"
#include "OperationTypes.h"
#include "Result.h"
#include "SharedMemory.h"
#include "TypeUtils.h"
#include "Types.h"

// The NN_VALIDATE family of macros defined below is similar to the CHECK family defined in
// system/libbase/include/android-base/logging.h
//
// The difference is that NN_VALIDATE macros use LOG(ERROR) instead of LOG(FATAL)
// and return false instead of aborting.

// Logs an error and returns false or INVALID. Append context using << after. For example:
//
//   NN_VALIDATE_FAIL() << "Something went wrong";
//
// The containing function must return a bool or Version.
#define NN_VALIDATE_FAIL() \
    return NN_ERROR() << "NN_VALIDATE failed (" << __FILE__ << ":" << __LINE__ << "): "

// Logs an error and returns false or Version::INVALID if condition is false. Extra logging can be
// appended using << after. For example:
//
//   NN_VALIDATE(false) << "Something went wrong";
//
// The containing function must return a bool.
#define NN_VALIDATE(condition) \
    while (UNLIKELY(!(condition))) NN_VALIDATE_FAIL() << #condition << " "

// Helper for NN_VALIDATE_xx(x, y) macros.
#define NN_VALIDATE_OP(LHS, RHS, OP)                                                        \
    for (auto _values = ::android::base::MakeEagerEvaluator(LHS, RHS);                      \
         UNLIKELY(!(_values.lhs.v OP _values.rhs.v));                                       \
         /* empty */)                                                                       \
    NN_VALIDATE_FAIL()                                                                      \
            << #LHS << " " << #OP << " " << #RHS << " (" << #LHS << " = "                   \
            << ::android::base::LogNullGuard<decltype(_values.lhs.v)>::Guard(_values.lhs.v) \
            << ", " << #RHS << " = "                                                        \
            << ::android::base::LogNullGuard<decltype(_values.rhs.v)>::Guard(_values.rhs.v) \
            << ") "

// Logs an error and returns false or Version::INVALID if a condition between x and y does not hold.
// Extra logging can be appended using << after. For example:
//
//   NN_VALIDATE_EQ(a, b) << "Something went wrong";
//
// The values must implement the appropriate comparison operator as well as
// `operator<<(std::ostream&, ...)`.
// The containing function must return a bool or Version.
#define NN_VALIDATE_EQ(x, y) NN_VALIDATE_OP(x, y, ==)
#define NN_VALIDATE_NE(x, y) NN_VALIDATE_OP(x, y, !=)
#define NN_VALIDATE_LE(x, y) NN_VALIDATE_OP(x, y, <=)
#define NN_VALIDATE_LT(x, y) NN_VALIDATE_OP(x, y, <)
#define NN_VALIDATE_GE(x, y) NN_VALIDATE_OP(x, y, >=)
#define NN_VALIDATE_GT(x, y) NN_VALIDATE_OP(x, y, >)

namespace android::nn {
namespace {

constexpr auto kNullptrVariant = std::variant<const void*, void*>{};
constexpr auto kInvalidMemoryDomainToken = Request::MemoryDomainToken{};

template <typename Type, typename ValidationFunction>
Result<Version> validateVector(const std::vector<Type>& objects,
                               const ValidationFunction& validationFunction) {
    auto version = Version::ANDROID_OC_MR1;
    for (const auto& object : objects) {
        version = combineVersions(version, NN_TRY(validationFunction(object)));
    }
    return version;
}

bool isValidExtensionName(const std::string& name) {
    constexpr auto validSymbol = [](char symbol) {
        return std::islower(symbol) || std::isdigit(symbol) || symbol == '.' || symbol == '_';
    };
    const bool hasOnlyValidSymbols = std::all_of(name.begin(), name.end(), validSymbol);
    const bool hasAtLeastOnePeriod = std::find(name.begin(), name.end(), '.') != name.end();
    return hasOnlyValidSymbols && hasAtLeastOnePeriod;
}

Result<Version> validateDeviceStatus(const DeviceStatus& deviceStatus) {
    switch (deviceStatus) {
        case DeviceStatus::AVAILABLE:
        case DeviceStatus::BUSY:
        case DeviceStatus::OFFLINE:
        case DeviceStatus::UNKNOWN:
            return Version::ANDROID_OC_MR1;
    }
    NN_VALIDATE_FAIL() << "Invalid DeviceStatus " << deviceStatus;
}

Result<Version> validateExecutionPreference(const ExecutionPreference& executionPreference) {
    switch (executionPreference) {
        case ExecutionPreference::FAST_SINGLE_ANSWER:
            // ExecutionPreference::FAST_SINGLE_ANSWER is the default value, so it is implicitly
            // valid for all versions.
            return Version::ANDROID_OC_MR1;
        case ExecutionPreference::LOW_POWER:
        case ExecutionPreference::SUSTAINED_SPEED:
            return Version::ANDROID_P;
    }
    NN_VALIDATE_FAIL() << "Invalid ExecutionPreference " << executionPreference;
}

Result<Version> validateDeviceType(const DeviceType& deviceType) {
    switch (deviceType) {
        case DeviceType::UNKNOWN:
            // DeviceType was introduced in the 1.2 NN HAL. DeviceType::UNKNOWN is returned when
            // querying versions that are prior to the 1.2 NN HAL. DeviceType::UNKNOWN is not a
            // valid code to return for a driver that implement at least a 1.2 NN HAL. If we need a
            // range of versions, make ANDROID_Q (NN HAL 1.2) the exclusive upper bound for
            // DeviceType::UNKNOWN.
            return Version::ANDROID_OC_MR1;
        case DeviceType::OTHER:
        case DeviceType::CPU:
        case DeviceType::GPU:
        case DeviceType::ACCELERATOR:
            return Version::ANDROID_Q;
    }
    NN_VALIDATE_FAIL() << "Invalid DeviceType " << deviceType;
}

Result<Version> validateMeasureTiming(const MeasureTiming& measureTiming) {
    switch (measureTiming) {
        case MeasureTiming::NO:
            // MeasureTiming::NO is the default value, so it is implicitly valid for all versions.
            return Version::ANDROID_OC_MR1;
        case MeasureTiming::YES:
            return Version::ANDROID_Q;
    }
    NN_VALIDATE_FAIL() << "Invalid MeasureTiming " << measureTiming;
}

Result<Version> validateOperandType(const OperandType& operandType) {
    switch (operandType) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
            return Version::ANDROID_OC_MR1;
        case OperandType::BOOL:
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_BOOL8:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT16_ASYMM:
        case OperandType::TENSOR_QUANT8_SYMM:
            return Version::ANDROID_Q;
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case OperandType::SUBGRAPH:
            return Version::ANDROID_R;
    }
    if (isExtension(operandType)) {
        return Version::ANDROID_Q;
    }
    NN_VALIDATE_FAIL() << "Invalid OperandType " << operandType;
}

Result<Version> validateOperandLifeTime(const Operand& operand) {
    // Make sure SUBGRAPH operand type and lifetime always go together.
    NN_VALIDATE_EQ((operand.type == OperandType::SUBGRAPH),
                   (operand.lifetime == Operand::LifeTime::SUBGRAPH))
            << "Operand of type " << operand.type << " cannot have lifetime " << operand.lifetime;

    switch (operand.lifetime) {
        case Operand::LifeTime::TEMPORARY_VARIABLE:
        case Operand::LifeTime::SUBGRAPH_INPUT:
        case Operand::LifeTime::SUBGRAPH_OUTPUT:
        case Operand::LifeTime::CONSTANT_COPY:
        case Operand::LifeTime::CONSTANT_REFERENCE:
        case Operand::LifeTime::NO_VALUE:
        case Operand::LifeTime::POINTER:
            return Version::ANDROID_OC_MR1;
        case Operand::LifeTime::SUBGRAPH:
            return Version::ANDROID_R;
    }
    NN_VALIDATE_FAIL() << "Invalid Operand::LifeTime " << operand.lifetime;
}

Result<Version> validatePriority(const Priority& priority) {
    switch (priority) {
        case Priority::MEDIUM:
            // Priority::MEDIUM is the default value, so it is implicitly valid for all versions.
            return Version::ANDROID_OC_MR1;
        case Priority::LOW:
        case Priority::HIGH:
            return Version::ANDROID_R;
    }
    NN_VALIDATE_FAIL() << "Invalid Priority " << priority;
}

Result<Version> validateErrorStatus(const ErrorStatus& errorStatus) {
    // Note that MISSED_DEADLINE_*, RESOURCE_EXHAUSTED_*, and DEAD_OBJECT were introduced ih
    // ANDROID_R, but these can be cast to ANDROID_OC_MR1 as GENERAL_FAILURE.
    switch (errorStatus) {
        case ErrorStatus::NONE:
        case ErrorStatus::DEVICE_UNAVAILABLE:
        case ErrorStatus::GENERAL_FAILURE:
        case ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
        case ErrorStatus::INVALID_ARGUMENT:
        case ErrorStatus::MISSED_DEADLINE_TRANSIENT:
        case ErrorStatus::MISSED_DEADLINE_PERSISTENT:
        case ErrorStatus::RESOURCE_EXHAUSTED_TRANSIENT:
        case ErrorStatus::RESOURCE_EXHAUSTED_PERSISTENT:
        case ErrorStatus::DEAD_OBJECT:
            return Version::ANDROID_OC_MR1;
    }
    NN_VALIDATE_FAIL() << "Invalid ErrorStatus " << errorStatus;
}

Result<Version> validateFusedActivationFunc(const FusedActivationFunc& activation) {
    switch (activation) {
        case FusedActivationFunc::NONE:
        case FusedActivationFunc::RELU:
        case FusedActivationFunc::RELU1:
        case FusedActivationFunc::RELU6:
            return Version::ANDROID_OC_MR1;
    }
    NN_VALIDATE_FAIL() << "Invalid FusedActivationFunc " << activation;
}

Result<Version> validateOutputShape(const OutputShape& /*outputShape*/) {
    return Version::ANDROID_Q;
}

Result<Version> validateTiming(const Timing& timing) {
    constexpr auto kNoTiming = Timing{};
    if (timing == kNoTiming) {
        // kNoTiming is the default value, so it is implicitly valid for all versions.
        return Version::ANDROID_OC_MR1;
    }
    if (timing.timeInDriver.has_value() && timing.timeOnDevice.has_value()) {
        // `lazyMessage` is a lazy function to produce the timing validation error message.
        // Currently, the code is not able to inline the message in NN_VALIDATE due to a
        // argument-dependent lookup issue with nn::detail::ErrorBuilder interacting with std types
        // such as std::chrono::duration, so this function uses an indirection through
        // std::ostringstream.
        const auto lazyMessage = [&timing]() -> std::string {
            std::ostringstream oss;
            oss << "Timing::timeOnDevice (" << timing.timeOnDevice.value()
                << ") must not exceed Timing::timeInDriver (" << timing.timeInDriver.value() << ")";
            return oss.str();
        };
        NN_VALIDATE(timing.timeOnDevice.value() <= timing.timeInDriver.value()) << lazyMessage();
    }
    return Version::ANDROID_Q;
}

Result<Version> validateCapabilitiesPerformanceInfo(
        const Capabilities::PerformanceInfo& performanceInfo) {
    NN_VALIDATE_GT(performanceInfo.execTime, 0.0f);
    NN_VALIDATE_GT(performanceInfo.powerUsage, 0.0f);
    return Version::ANDROID_OC_MR1;
}

Result<Version> validateCapabilitiesOperandPerformance(
        const Capabilities::OperandPerformance& operandPerformance) {
    auto version = NN_TRY(validateOperandType(operandPerformance.type));
    return combineVersions(version,
                           NN_TRY(validateCapabilitiesPerformanceInfo(operandPerformance.info)));
}

Result<Version> validateCapabilitiesOperandPerformanceTable(
        const Capabilities::OperandPerformanceTable& operandPerformances) {
    // OperandPerformanceTable's order was validated when it was created, and it is castable to any
    // version. If an OperandType does not exist in the lower version being converted to, that
    // OperandPerformance will be dropped.
    NN_TRY(validateVector(operandPerformances.asVector(), validateCapabilitiesOperandPerformance));
    return Version::ANDROID_OC_MR1;
}

Result<Version> validateCapabilities(const Capabilities& capabilities) {
    auto version =
            NN_TRY(validateCapabilitiesOperandPerformanceTable(capabilities.operandPerformance));

    version = combineVersions(version,
                              NN_TRY(validateCapabilitiesPerformanceInfo(
                                      capabilities.relaxedFloat32toFloat16PerformanceScalar)));
    version = combineVersions(version,
                              NN_TRY(validateCapabilitiesPerformanceInfo(
                                      capabilities.relaxedFloat32toFloat16PerformanceTensor)));
    version = combineVersions(
            version, NN_TRY(validateCapabilitiesPerformanceInfo(capabilities.ifPerformance)));
    version = combineVersions(
            version, NN_TRY(validateCapabilitiesPerformanceInfo(capabilities.whilePerformance)));

    return version;
}

Result<Version> validateExtensionOperandTypeInformation(
        const Extension::OperandTypeInformation& operandTypeInformation) {
    NN_VALIDATE_GT(operandTypeInformation.byteSize, 0u);
    return Version::ANDROID_Q;
}

Result<Version> validateExtension(const Extension& extension) {
    NN_VALIDATE(isValidExtensionName(extension.name));

    // Verify all OperandTypeInformations have unique types.
    std::vector<uint16_t> types;
    types.reserve(extension.operandTypes.size());
    std::transform(extension.operandTypes.begin(), extension.operandTypes.end(),
                   std::back_inserter(types),
                   [](const Extension::OperandTypeInformation& operandTypeInformation) {
                       return operandTypeInformation.type;
                   });
    std::sort(types.begin(), types.end());
    const auto iter = std::adjacent_find(types.begin(), types.end());
    NN_VALIDATE(iter == types.end()) << "Extension has duplicate type " << *iter;

    return combineVersions(Version::ANDROID_Q,
                           NN_TRY(validateVector(extension.operandTypes,
                                                 validateExtensionOperandTypeInformation)));
}

Result<Version> validateExtensions(const std::vector<Extension>& extensions) {
    const auto version = NN_TRY(validateVector(extensions, validateExtension));

    // Verify all extensions have unique names.
    std::vector<std::reference_wrapper<const std::string>> names;
    names.reserve(extensions.size());
    std::transform(extensions.begin(), extensions.end(), std::back_inserter(names),
                   [](const Extension& extension) { return std::cref(extension.name); });
    std::sort(names.begin(), names.end(), std::less<std::string>{});
    const auto nameIter =
            std::adjacent_find(names.begin(), names.end(), std::equal_to<std::string>{});
    NN_VALIDATE(nameIter == names.end())
            << "Two or more extensions have the duplicate name " << nameIter->get();

    return version;
}

// Forward declaration of subgraph validation function.
Result<Version> validateModelSubgraph(const Model::Subgraph& subgraph,
                                      std::optional<size_t> referencedIndex,
                                      size_t operandValuesSize,
                                      const std::vector<size_t>& poolSizes,
                                      const std::vector<Model::Subgraph>& referenced,
                                      std::vector<std::optional<Version>>* subgraphVersionCache);

Result<Version> validateOperandDataLocation(
        const Operand& operand, size_t operandValuesSize, const std::vector<size_t>& poolSizes,
        const std::vector<Model::Subgraph>& subgraphs,
        std::vector<std::optional<Version>>* subgraphVersionCache) {
    const DataLocation& location = operand.location;
    NN_VALIDATE_EQ(location.padding, 0u)
            << "DataLocation with a non-zero padding used in Model: " << location.padding;
    switch (operand.lifetime) {
        case Operand::LifeTime::CONSTANT_COPY:
            NN_VALIDATE(location.pointer == kNullptrVariant)
                    << "CONSTANT_COPY with a non-null pointer";
            NN_VALIDATE_EQ(location.poolIndex, 0u)
                    << "CONSTANT_COPY with a non-zero poolIndex " << location.poolIndex;
            // Do the addition using uint64_t to avoid potential wrap-around problems.
            NN_VALIDATE_LE(static_cast<uint64_t>(location.offset) + location.length,
                           operandValuesSize)
                    << "OperandValue location out of range.  Starts at " << location.offset
                    << ", length " << location.length << ", max " << operandValuesSize;
            return Version::ANDROID_OC_MR1;
        case Operand::LifeTime::CONSTANT_REFERENCE:
            NN_VALIDATE_LT(location.poolIndex, poolSizes.size());
            // Do the addition using uint64_t to avoid potential wrap-around problems.
            NN_VALIDATE_LE(static_cast<uint64_t>(location.offset) + location.length,
                           poolSizes[location.poolIndex])
                    << "OperandValue location out of range.  Starts at " << location.offset
                    << ", length " << location.length << ", max " << poolSizes[location.poolIndex];
            return Version::ANDROID_OC_MR1;
        case Operand::LifeTime::TEMPORARY_VARIABLE:
        case Operand::LifeTime::SUBGRAPH_INPUT:
        case Operand::LifeTime::SUBGRAPH_OUTPUT:
        case Operand::LifeTime::NO_VALUE:
            NN_VALIDATE(location.pointer == kNullptrVariant)
                    << "Unexpected pointer value for operand of lifetime " << operand.lifetime;
            NN_VALIDATE_EQ(location.poolIndex, 0u)
                    << "Unexpected poolIndex " << location.poolIndex << " for operand of lifetime "
                    << operand.lifetime;
            NN_VALIDATE_EQ(location.offset, 0u) << "Unexpected offset " << location.offset
                                                << " for operand of lifetime " << operand.lifetime;
            NN_VALIDATE_EQ(location.length, 0u) << "Unexpected length " << location.length
                                                << " for operand of lifetime " << operand.lifetime;
            return Version::ANDROID_OC_MR1;
        case Operand::LifeTime::SUBGRAPH: {
            NN_VALIDATE(location.pointer == kNullptrVariant) << "SUBGRAPH with a non-null pointer";
            NN_VALIDATE_EQ(location.poolIndex, 0u)
                    << "SUBGRAPH with a non-zero poolIndex " << location.poolIndex;
            NN_VALIDATE_LT(location.offset, subgraphs.size())
                    << "Subgraph index out of range: " << location.offset
                    << " >= " << subgraphs.size();
            NN_VALIDATE_EQ(location.length, 0u)
                    << "SUBGRAPH with a non-zero length " << location.length;
            const auto version = NN_TRY(validateModelSubgraph(
                    subgraphs[location.offset], location.offset, operandValuesSize, poolSizes,
                    subgraphs, subgraphVersionCache));
            return combineVersions(version, Version::ANDROID_R);
        }
        case Operand::LifeTime::POINTER: {
            const bool nonNull =
                    std::visit([](auto* ptr) { return ptr != nullptr; }, location.pointer);
            NN_VALIDATE(nonNull) << "POINTER with a null pointer";
            NN_VALIDATE_EQ(location.poolIndex, 0u)
                    << "POINTER with a non-zero poolIndex " << location.poolIndex;
            NN_VALIDATE_EQ(location.offset, 0u)
                    << "POINTER with a non-zero offset " << location.offset;
            return Version::ANDROID_OC_MR1;
        }
    }
    NN_VALIDATE_FAIL() << "Invalid Operand::LifeTime " << operand.lifetime;
}

Result<Version> validateOperandDimensions(const Operand& operand) {
    switch (operand.type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::BOOL:
        case OperandType::FLOAT16:
        case OperandType::SUBGRAPH:
        case OperandType::OEM:
            NN_VALIDATE(operand.dimensions.empty())
                    << "Scalar data has dimensions of rank " << operand.dimensions.size();
            return Version::ANDROID_OC_MR1;
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
        case OperandType::TENSOR_OEM_BYTE: {
            if (operand.lifetime == Operand::LifeTime::CONSTANT_COPY ||
                operand.lifetime == Operand::LifeTime::CONSTANT_REFERENCE ||
                operand.lifetime == Operand::LifeTime::POINTER) {
                NN_VALIDATE(!operand.dimensions.empty())
                        << "Tensor has lifetime of " << operand.lifetime
                        << " but dimensions of rank 0";
                const auto size = getNonExtensionSize(operand);
                NN_VALIDATE(size.has_value()) << "Tensor dimensions overflow";
                NN_VALIDATE_NE(size.value(), 0u) << "Tensor has at least one unknown dimension";
            }
            // TODO(b/165152547): aren't NO_VALUE arguments allowed to be .empty() even before
            // Android Q?
            if (operand.dimensions.empty()) {
                // Unspecified rank was added in Android Q.
                return Version::ANDROID_Q;
            }
            return Version::ANDROID_OC_MR1;
        }
    }
    if (isExtension(operand.type)) {
        // Extension types were added in Android Q.
        return Version::ANDROID_Q;
    }
    NN_VALIDATE_FAIL() << "Invalid OperandType " << operand.type;
}

Result<Version> validateOperandScale(const Operand& operand) {
    switch (operand.type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::BOOL:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_BOOL8:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::SUBGRAPH:
            NN_VALIDATE_EQ(operand.scale, 0.0f)
                    << "Operand of type " << operand.type << " with a non-zero scale ("
                    << operand.scale << ")";
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_INT32:
            // TENSOR_INT32 may be used with or without scale, depending on the operation.
            // TODO(b/119869082) We should have a separate type for TENSOR_INT32 with a scale.
            NN_VALIDATE_GE(operand.scale, 0.0f)
                    << "Operand of type " << operand.type << " with a negative scale";
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_QUANT16_ASYMM:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            NN_VALIDATE_GT(operand.scale, 0.0f)
                    << "Operand of type " << operand.type << " with a non-positive scale";
            return Version::ANDROID_OC_MR1;
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
            // No validation for OEM types.
            return Version::ANDROID_OC_MR1;
    }
    if (isExtension(operand.type)) {
        NN_VALIDATE_EQ(operand.scale, 0.0f) << "Operand of type " << operand.type
                                            << " with a non-zero scale (" << operand.scale << ")";
        return Version::ANDROID_Q;
    }
    NN_VALIDATE_FAIL() << "Invalid OperandType " << operand.type;
}

Result<Version> validateOperandZeroPoint(const Operand& operand) {
    switch (operand.type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::BOOL:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_BOOL8:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::SUBGRAPH:
            NN_VALIDATE_EQ(operand.zeroPoint, 0)
                    << "Operand of type " << operand.type << " with a non-zero zeroPoint "
                    << operand.zeroPoint;
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_QUANT8_ASYMM:
            NN_VALIDATE(operand.zeroPoint >= 0 && operand.zeroPoint <= 255)
                    << "Operand of type " << operand.type << " with an invalid zeroPoint "
                    << operand.zeroPoint << ", must be in range [0, 255]";
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            NN_VALIDATE(operand.zeroPoint >= -128 && operand.zeroPoint <= 127)
                    << "Operand of type " << operand.type << " with an invalid zeroPoint "
                    << operand.zeroPoint << ", must be in range [-128, 127]";
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_QUANT16_ASYMM:
            NN_VALIDATE(operand.zeroPoint >= 0 && operand.zeroPoint <= 65535)
                    << "Operand of type " << operand.type << " with an invalid zeroPoint "
                    << operand.zeroPoint << ", must be in range [0, 65535]";
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_QUANT16_SYMM:
            NN_VALIDATE_EQ(operand.zeroPoint, 0)
                    << "Operand of type " << operand.type << " with a non-zero zeroPoint "
                    << operand.zeroPoint;
            return Version::ANDROID_OC_MR1;
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
            // No validation for OEM types.
            return Version::ANDROID_OC_MR1;
    }
    if (isExtension(operand.type)) {
        NN_VALIDATE_EQ(operand.zeroPoint, 0) << "Operand of type " << operand.type
                                             << " with a non-zero zeroPoint " << operand.zeroPoint;
        return Version::ANDROID_Q;
    }
    NN_VALIDATE_FAIL() << "Invalid OperandType " << operand.type;
}

Result<Version> validateOperandExtraParams(const Operand& operand) {
    switch (operand.type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::BOOL:
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_BOOL8:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT16_ASYMM:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case OperandType::SUBGRAPH:
            NN_VALIDATE(std::holds_alternative<Operand::NoParams>(operand.extraParams))
                    << "Operand of type " << operand.type
                    << " has extraParams when there must be none";
            return Version::ANDROID_OC_MR1;
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL: {
            NN_VALIDATE(
                    std::holds_alternative<Operand::SymmPerChannelQuantParams>(operand.extraParams))
                    << "Operand of type " << operand.type
                    << " without a Channel Quantization params";
            const auto& channelQuant =
                    std::get<Operand::SymmPerChannelQuantParams>(operand.extraParams);

            const size_t count = operand.dimensions.size();
            NN_VALIDATE_LT(channelQuant.channelDim, count)
                    << "Operand of type " << operand.type
                    << " with an invalid channelQuant.channelDim " << channelQuant.channelDim
                    << ", must be valid dimension index in range [0, " << count << ")";
            const uint32_t expected = operand.dimensions[channelQuant.channelDim];
            NN_VALIDATE_EQ(channelQuant.scales.size(), expected)
                    << "Operand of type " << operand.type << " with a wrong-sized scales, expected "
                    << expected << " was " << channelQuant.scales.size();
            NN_VALIDATE_NE(expected, 0u)
                    << "Operand of type " << operand.type << " channel dimension "
                    << channelQuant.channelDim << " is underspecified (can't be 0)";
            for (uint32_t i = 0; i < expected; ++i) {
                NN_VALIDATE_GT(channelQuant.scales[i], 0.0f)
                        << "Operand of type " << operand.type
                        << " with a non-positive value in scales[" << i
                        << "]=" << channelQuant.scales[i];
            }
            return Version::ANDROID_Q;
        }
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
            // No validation for OEM types.
            return Version::ANDROID_OC_MR1;
    }
    if (isExtension(operand.type)) {
        NN_VALIDATE(std::holds_alternative<Operand::NoParams>(operand.extraParams) ||
                    std::holds_alternative<Operand::ExtensionParams>(operand.extraParams))
                << "Extension operand of type " << operand.type
                << " must not have SymmPerChannelQuant extraParams";
        return Version::ANDROID_OC_MR1;
    }
    NN_VALIDATE_FAIL() << "Invalid OperandType " << operand.type;
}

Result<Version> validateOperand(const Operand& operand, size_t operandValuesSize,
                                const std::vector<size_t>& poolSizes,
                                const std::vector<Model::Subgraph>& subgraphs,
                                std::vector<std::optional<Version>>* subgraphVersionCache) {
    auto version = NN_TRY(validateOperandType(operand.type));
    version = combineVersions(version, NN_TRY(validateOperandLifeTime(operand)));
    version = combineVersions(version, NN_TRY(validateOperandDimensions(operand)));
    version = combineVersions(version, NN_TRY(validateOperandScale(operand)));
    version = combineVersions(version, NN_TRY(validateOperandZeroPoint(operand)));
    version = combineVersions(version, NN_TRY(validateOperandExtraParams(operand)));
    version = combineVersions(
            version, NN_TRY(validateOperandDataLocation(operand, operandValuesSize, poolSizes,
                                                        subgraphs, subgraphVersionCache)));

    // For constants, validate that the length is as expected. The other lifetimes
    // expect the length to be 0. Don't validate for OEM types.
    if (operand.lifetime == Operand::LifeTime::CONSTANT_REFERENCE ||
        operand.lifetime == Operand::LifeTime::CONSTANT_COPY ||
        operand.lifetime == Operand::LifeTime::POINTER) {
        if (!isExtension(operand.type) && operand.type != OperandType::OEM &&
            operand.type != OperandType::TENSOR_OEM_BYTE) {
            const auto expectedLength = getNonExtensionSize(operand).value();
            NN_VALIDATE_EQ(operand.location.length, expectedLength)
                    << "For operand " << operand.type << " expected a size of " << expectedLength
                    << " but got " << operand.location.length;
        }
    }

    return version;
}

Result<std::vector<Version>> validateOperands(
        const std::vector<Operand>& operands, size_t operandValuesSize,
        const std::vector<size_t>& poolSizes, const std::vector<Model::Subgraph>& subgraphs,
        std::vector<std::optional<Version>>* subgraphVersionCache) {
    std::vector<Version> versions;
    versions.reserve(operands.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        auto result = validateOperand(operands[i], operandValuesSize, poolSizes, subgraphs,
                                      subgraphVersionCache);
        if (!result.has_value()) {
            return error() << std::move(result).error() << " for operand " << i;
        }
        versions.push_back(result.value());
    }
    return versions;
}

// Forward declaration.
Result<Version> validateOperationIncludingOperandVersions(
        const Operation& operation, const std::vector<Operand>& operands,
        const std::vector<Version>& operandVersions, const std::vector<Model::Subgraph>& subgraphs);

Result<Version> validateOperations(const std::vector<Operation>& operations,
                                   const std::vector<Operand>& operands,
                                   const std::vector<Version>& operandVersions,
                                   const std::vector<Model::Subgraph>& subgraphs) {
    auto version = Version::ANDROID_OC_MR1;
    for (size_t i = 0; i < operations.size(); ++i) {
        auto result = validateOperationIncludingOperandVersions(operations[i], operands,
                                                                operandVersions, subgraphs);
        if (!result.has_value()) {
            return error() << std::move(result).error() << " for operation " << i;
        }
        version = combineVersions(version, result.value());
    }
    return version;
}

Result<Version> validateHandle(const Handle& handle) {
    NN_VALIDATE(std::all_of(handle.fds.begin(), handle.fds.end(),
                            [](const base::unique_fd& fd) { return fd.ok(); }));
    return Version::ANDROID_OC_MR1;
}

Result<Version> validateSharedHandle(const SharedHandle& handle) {
    NN_VALIDATE(handle != nullptr);
    return validateHandle(*handle);
}

Result<Version> validateMemory(const Memory::Ashmem& memory) {
    NN_VALIDATE(memory.fd.ok());
    NN_VALIDATE_NE(memory.size, 0u);
    return Version::ANDROID_OC_MR1;
}

Result<Version> validateMemory(const Memory::Fd& memory) {
    NN_VALIDATE(memory.fd.ok());
    NN_VALIDATE_NE(memory.size, 0u);

    // `prot` is allowed to be either PROT_NONE (which has a value of 0) or the bitwise OR of either
    // PROT_READ or PROT_WRITE. If any other bits are set, the `prot` field is invalid.
    constexpr int kAllowedBits = PROT_READ | PROT_WRITE;
    NN_VALIDATE_EQ(memory.prot & ~kAllowedBits, 0);

    return Version::ANDROID_OC_MR1;
}

Result<Version> validateMemory(const Memory::HardwareBuffer& memory) {
    NN_VALIDATE(memory.handle.get() != nullptr);
    return Version::ANDROID_Q;
}

Result<Version> validateMemory(const Memory::Unknown& memory) {
    NN_TRY(validateHandle(memory.handle));
    return Version::ANDROID_Q;
}

Result<Version> validateSharedMemory(const SharedMemory& memory) {
    NN_VALIDATE(memory != nullptr);
    return std::visit([](const auto& x) { return validateMemory(x); }, memory->handle);
}

Result<void> validateModelSubgraphInputOutputs(const std::vector<uint32_t>& indexes,
                                               const std::vector<Operand>& operands,
                                               Operand::LifeTime lifetime) {
    const size_t operandCount = operands.size();
    for (uint32_t i : indexes) {
        NN_VALIDATE_LT(i, operandCount)
                << "Model " << lifetime << " input or output index out of range: " << i << "/"
                << operandCount;
        const Operand& operand = operands[i];
        NN_VALIDATE_EQ(operand.lifetime, lifetime)
                << "Model " << lifetime << " operand " << i << " has lifetime of "
                << operand.lifetime << " instead of the expected " << lifetime;
    }

    std::vector<uint32_t> sortedIndexes = indexes;
    std::sort(sortedIndexes.begin(), sortedIndexes.end());
    const auto iter = std::adjacent_find(sortedIndexes.begin(), sortedIndexes.end());
    NN_VALIDATE(iter == sortedIndexes.end())
            << "Model input or output occurs multiple times: " << *iter;

    for (size_t i = 0; i < operands.size(); ++i) {
        if (operands[i].lifetime == lifetime) {
            const auto containsIndex = [&sortedIndexes](size_t index) {
                return binary_search(sortedIndexes.begin(), sortedIndexes.end(), index);
            };
            NN_VALIDATE(containsIndex(i))
                    << "Operand " << i << " marked as " << lifetime
                    << " but is not included in Model input or output indexes";
        }
    }

    return {};
}

Result<void> validateExecutionOrder(const Model::Subgraph& subgraph) {
    // Either the operand has a known value before model execution begins, or we've seen a writer
    // for this operand while walking operands in execution order. Initialize to known operands.
    std::vector<bool> operandValueKnown;
    operandValueKnown.reserve(subgraph.operands.size());
    std::transform(subgraph.operands.begin(), subgraph.operands.end(),
                   std::back_inserter(operandValueKnown), [](const Operand& operand) {
                       return operand.lifetime != Operand::LifeTime::TEMPORARY_VARIABLE &&
                              operand.lifetime != Operand::LifeTime::SUBGRAPH_OUTPUT;
                   });

    // Validate that operations are sorted into execution order.
    //
    // If there is a cycle in the graph, the operations will not
    // appear to be sorted into execution order: Some operation will
    // have an input for which operandValueKnown[] is false.
    for (size_t i = 0; i < subgraph.operations.size(); ++i) {
        const auto& operation = subgraph.operations[i];

        for (size_t j = 0; j < operation.inputs.size(); ++j) {
            const uint32_t k = operation.inputs[j];
            NN_VALIDATE(operandValueKnown[k]) << "Operation " << i << " input " << j << " (operand "
                                              << k << ") is read before it is written";
        }

        for (size_t j = 0; j < operation.outputs.size(); ++j) {
            const uint32_t k = operation.outputs[j];
            // Assuming validateOperations() has not returned an error, we know that this output is
            // TEMPORARY_VARIABLE or MODEL_OUTPUT, and so the only way operandValueKnown[k] can be
            // true is if we've already seen a writer for this operand.
            NN_VALIDATE(!operandValueKnown[k]) << "Operation " << i << " output " << j
                                               << " (operand " << k << ") has already been written";
            operandValueKnown[k] = true;
        }
    }

    // Verify all operands are written.
    for (size_t i = 0; i < subgraph.operands.size(); ++i) {
        NN_VALIDATE(operandValueKnown[i]) << "Operand " << i << " is never written";
    }

    // TODO(b/77871786): verify that every operation has at least one output operand that is read?

    return {};
}

// Validate a subgraph, ensuring all subgraphs it depends on are also validated.
//
// `referencedIndex` is empty if the subgraph being validated is the main subgraph, otherwise it is
// the index of the referenced subgraph being validated.
//
// referenced[i] and (*subgraphVersionCache)[i] correspond to the same subgraph, and therefore
// `referenced` and `subgraphVersionCache` must have the same length.
Result<Version> validateModelSubgraph(const Model::Subgraph& subgraph,
                                      std::optional<size_t> referencedIndex,
                                      size_t operandValuesSize,
                                      const std::vector<size_t>& poolSizes,
                                      const std::vector<Model::Subgraph>& referenced,
                                      std::vector<std::optional<Version>>* subgraphVersionCache) {
    CHECK(subgraphVersionCache != nullptr);
    CHECK_EQ(referenced.size(), subgraphVersionCache->size());

    // Quickly return if the current subgraph has already been checked for its version.
    if (referencedIndex.has_value()) {
        if (auto version = subgraphVersionCache->at(*referencedIndex)) {
            return *version;
        }
    }

    NN_VALIDATE(!subgraph.operands.empty());
    NN_VALIDATE(!subgraph.operations.empty());
    // TODO(b/173780642): Clarify whether subgraphs with no inputs or outputs are valid.
    // NN_VALIDATE(!subgraph.inputIndexes.empty());
    // NN_VALIDATE(!subgraph.outputIndexes.empty());

    const auto operandVersions = NN_TRY(validateOperands(
            subgraph.operands, operandValuesSize, poolSizes, referenced, subgraphVersionCache));
    const auto operationsVersion = NN_TRY(validateOperations(subgraph.operations, subgraph.operands,
                                                             operandVersions, referenced));

    // Accumulate the versions from all operands and operations.
    const auto version = std::accumulate(operandVersions.begin(), operandVersions.end(),
                                         operationsVersion, combineVersions);

    NN_TRY(validateModelSubgraphInputOutputs(subgraph.inputIndexes, subgraph.operands,
                                             Operand::LifeTime::SUBGRAPH_INPUT));
    NN_TRY(validateModelSubgraphInputOutputs(subgraph.outputIndexes, subgraph.operands,
                                             Operand::LifeTime::SUBGRAPH_OUTPUT));

    NN_TRY(validateExecutionOrder(subgraph));

    // Mark the current subgraph as having already been validated so the caller can quickly return
    // if this subgraph is checked again.
    if (referencedIndex.has_value()) {
        subgraphVersionCache->at(*referencedIndex) = version;
    }
    return version;
}

Result<Version> validateModelExtensionNamesAndPrefixes(
        const std::vector<Model::ExtensionNameAndPrefix>& extensionNamesAndPrefixes) {
    for (const auto& extensionNameAndPrefix : extensionNamesAndPrefixes) {
        NN_VALIDATE(isValidExtensionName(extensionNameAndPrefix.name));
    }

    std::vector<std::reference_wrapper<const std::string>> names;
    names.reserve(extensionNamesAndPrefixes.size());
    std::transform(extensionNamesAndPrefixes.begin(), extensionNamesAndPrefixes.end(),
                   std::back_inserter(names),
                   [](const Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
                       return std::cref(extensionNameAndPrefix.name);
                   });
    std::sort(names.begin(), names.end(), std::less<std::string>{});
    const auto nameIter =
            std::adjacent_find(names.begin(), names.end(), std::equal_to<std::string>{});
    NN_VALIDATE(nameIter == names.end())
            << "ExtensionNamesAndPrefixes has duplicate name " << nameIter->get();

    std::vector<uint16_t> types;
    types.reserve(extensionNamesAndPrefixes.size());
    std::transform(extensionNamesAndPrefixes.begin(), extensionNamesAndPrefixes.end(),
                   std::back_inserter(types),
                   [](const Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
                       return extensionNameAndPrefix.prefix;
                   });
    std::sort(types.begin(), types.end());
    const auto typeIter = std::adjacent_find(types.begin(), types.end());
    NN_VALIDATE(typeIter == types.end())
            << "ExtensionNamesAndPrefixes has duplicate type " << *typeIter;

    const bool hasExtensions = !extensionNamesAndPrefixes.empty();
    return hasExtensions ? Version::ANDROID_Q : Version::ANDROID_OC_MR1;
}

// Makes sure the model does not contain subgraph reference cycles.
//
// This function verifies that referencedSubgraphs[subgraphIndex] and any subgraphs it refences do
// not contain any reference cycles. `path` is used to keep track of which referenced subgraphs have
// already been visited in the current recursive reference path. `verified` is a cache to keep track
// of which referenced subgraphs have already been verified not to form reference cycles.
//
// referencedSubgraphs[i], (*path)[i], and (*verified)[i] all correspond to the same subgraph, and
// therefore `referencedSubgraphs`, `path`, and `verified` must all have the same length.
Result<void> checkNoReferenceCycles(const std::vector<Model::Subgraph>& referencedSubgraphs,
                                    uint32_t subgraphIndex, std::vector<bool>* path,
                                    std::vector<bool>* verified) {
    CHECK(path != nullptr);
    CHECK(verified != nullptr);
    CHECK_EQ(referencedSubgraphs.size(), path->size());
    CHECK_EQ(referencedSubgraphs.size(), verified->size());
    const auto& subgraph = referencedSubgraphs.at(subgraphIndex);

    // Quickly return if the current subgraph has already been verified to have no reference cycles.
    if ((*verified)[subgraphIndex]) {
        return {};
    }

    // Add the current subgraph to the path (making sure that it is not already part of the path),
    // and verify that all subgraphs this subgraph references do not contain cycles. The current
    // subgraph is removed from the path only after all subgraphs this subgraph references have been
    // checked.
    NN_VALIDATE((*path)[subgraphIndex] == false) << "Model contains a circular subgraph reference";
    (*path)[subgraphIndex] = true;
    for (const Operand& operand : subgraph.operands) {
        if (operand.lifetime == Operand::LifeTime::SUBGRAPH) {
            const uint32_t refSubgraphIndex = operand.location.offset;
            NN_TRY(checkNoReferenceCycles(referencedSubgraphs, refSubgraphIndex, path, verified));
        }
    }
    (*path)[subgraphIndex] = false;

    // Mark the current subgraph as having already been verified so the caller can quickly return if
    // this subgraph is checked again.
    (*verified)[subgraphIndex] = true;
    return {};
}

Result<void> checkNoReferenceCycles(const std::vector<Model::Subgraph>& referencedSubgraphs) {
    const size_t count = referencedSubgraphs.size();
    std::vector<bool> path(count);
    std::vector<bool> verified(count);
    for (size_t i = 0; i < count; ++i) {
        NN_TRY(checkNoReferenceCycles(referencedSubgraphs, i, &path, &verified));
    }
    return {};
}

Result<Version> validateModel(const Model& model) {
    auto version = NN_TRY(validateVector(model.pools, validateSharedMemory));
    version = combineVersions(
            version, NN_TRY(validateModelExtensionNamesAndPrefixes(model.extensionNameToPrefix)));

    // Ignore relaxComputationFloat32toFloat16 version because in the worst case it makes the
    // execution stricter.

    // Referenced models were introduced in Android R.
    const bool hasReferencedModels = !model.referenced.empty();
    const auto referenceModelVersion =
            hasReferencedModels ? Version::ANDROID_R : Version::ANDROID_OC_MR1;
    version = combineVersions(version, referenceModelVersion);

    // Ensure that there are no cycles formed by the subgraphs.
    NN_TRY(checkNoReferenceCycles(model.referenced));

    // Get memory sizes.
    const auto [operandValuesSize, poolSizes] = getMemorySizes(model);

    // Validate referenced subgraphs.
    auto subgraphVersionCache = std::vector<std::optional<Version>>(model.referenced.size());
    for (size_t referencedIndex = 0; referencedIndex < model.referenced.size(); ++referencedIndex) {
        const auto& subgraph = model.referenced[referencedIndex];
        const auto subgraphVersion =
                NN_TRY(validateModelSubgraph(subgraph, referencedIndex, operandValuesSize,
                                             poolSizes, model.referenced, &subgraphVersionCache));
        version = combineVersions(version, subgraphVersion);
    }

    // Validate main subgraph.
    const auto subgraphVersion =
            NN_TRY(validateModelSubgraph(model.main, std::nullopt, operandValuesSize, poolSizes,
                                         model.referenced, &subgraphVersionCache));
    version = combineVersions(version, subgraphVersion);

    return version;
}

Result<Version> validateBufferDesc(const BufferDesc& bufferDesc) {
    // An empty BufferDesc is the default value, so it is implicitly valid for all versions.
    return bufferDesc.dimensions.empty() ? Version::ANDROID_OC_MR1 : Version::ANDROID_R;
}

Result<Version> validateBufferRole(const BufferRole& bufferRole) {
    NN_VALIDATE_GT(bufferRole.probability, 0.0f);
    NN_VALIDATE_LE(bufferRole.probability, 1.0f);
    return Version::ANDROID_R;
}

Result<Version> validateRequestArgument(const Request::Argument& requestArgument,
                                        const std::vector<size_t>& memorySizes, bool isOutput) {
    const auto lifetime = requestArgument.lifetime;
    const auto& location = requestArgument.location;
    const auto& dimensions = requestArgument.dimensions;

    switch (lifetime) {
        case Request::Argument::LifeTime::POOL: {
            NN_VALIDATE(location.pointer == kNullptrVariant);
            NN_VALIDATE_LT(location.poolIndex, memorySizes.size());
            // Do the addition using uint64_t to avoid potential wrap-around problems.
            const auto lastPosition =
                    static_cast<uint64_t>(location.offset) + location.length + location.padding;
            const auto memorySize = memorySizes[location.poolIndex];
            NN_VALIDATE_LE(lastPosition, memorySize);
            if (memorySize > 0) {
                // Must specify a positive length if the memory pool has a known size.
                NN_VALIDATE_GT(location.length, 0u);
            }
            return Version::ANDROID_OC_MR1;
        }
        case Request::Argument::LifeTime::NO_VALUE:
            NN_VALIDATE(location.pointer == kNullptrVariant);
            NN_VALIDATE_EQ(location.poolIndex, 0u);
            NN_VALIDATE_EQ(location.offset, 0u);
            NN_VALIDATE_EQ(location.length, 0u);
            NN_VALIDATE_EQ(location.padding, 0u);
            NN_VALIDATE(dimensions.empty());
            return Version::ANDROID_OC_MR1;
        case Request::Argument::LifeTime::POINTER: {
            const bool isNullptr =
                    std::visit([](auto ptr) { return ptr == nullptr; }, location.pointer);
            NN_VALIDATE(!isNullptr);
            NN_VALIDATE_EQ(location.poolIndex, 0u);
            NN_VALIDATE_EQ(location.offset, 0u);
            NN_VALIDATE_NE(location.length, 0u);
            if (isOutput) {
                NN_VALIDATE(std::holds_alternative<void*>(location.pointer));
            }
            return Version::ANDROID_OC_MR1;
        }
    }
    NN_VALIDATE_FAIL() << "Invalid Request::Argument::LifeTime " << lifetime;
}

Result<Version> validateRequestMemoryPool(const Request::MemoryPool& memoryPool) {
    if (std::holds_alternative<Request::MemoryDomainToken>(memoryPool)) {
        NN_VALIDATE(std::get<Request::MemoryDomainToken>(memoryPool) != kInvalidMemoryDomainToken);
        return Version::ANDROID_R;
    }
    if (std::holds_alternative<SharedBuffer>(memoryPool)) {
        NN_VALIDATE(std::get<SharedBuffer>(memoryPool) != nullptr);
        return Version::ANDROID_R;
    }
    return validateSharedMemory(std::get<SharedMemory>(memoryPool));
}

Result<Version> validateRequest(const Request& request) {
    auto version = NN_TRY(validateVector(request.pools, validateRequestMemoryPool));

    // Get memory sizes. For IBuffer or MemoryDomainToken types, set size to 0.
    std::vector<size_t> memorySizes;
    memorySizes.reserve(request.pools.size());
    std::transform(request.pools.begin(), request.pools.end(), std::back_inserter(memorySizes),
                   [](const Request::MemoryPool& memoryPool) {
                       const auto* memory = std::get_if<SharedMemory>(&memoryPool);
                       return memory != nullptr ? getSize(*memory) : 0;
                   });

    for (size_t i = 0; i < request.inputs.size(); ++i) {
        const auto& input = request.inputs[i];
        auto result = validateRequestArgument(input, memorySizes, /*isOutput=*/false);
        if (!result.has_value()) {
            return error() << std::move(result).error() << " for input RequestArgument " << i;
        }
        version = combineVersions(version, result.value());
    }
    for (size_t i = 0; i < request.outputs.size(); ++i) {
        const auto& output = request.outputs[i];
        auto result = validateRequestArgument(output, memorySizes, /*isOutput=*/true);
        if (!result.has_value()) {
            return error() << std::move(result).error() << " for output RequestArgument " << i;
        }
        version = combineVersions(version, result.value());
    }

    return version;
}

Result<Version> validateOptionalTimePoint(const OptionalTimePoint& optionalTimePoint) {
    if (optionalTimePoint.has_value()) {
        NN_VALIDATE_GE(optionalTimePoint->time_since_epoch().count(), 0);
    }
    // An omitted time point is the default value, so it is implicitly valid for all versions.
    return !optionalTimePoint.has_value() ? Version::ANDROID_OC_MR1 : Version::ANDROID_R;
}

Result<Version> validateOptionalTimeoutDuration(const OptionalDuration& optionalTimeoutDuration) {
    if (optionalTimeoutDuration.has_value()) {
        NN_VALIDATE_GE(optionalTimeoutDuration->count(), 0);
    }
    // An omitted duration is the default value, so it is implicitly valid for all versions.
    return !optionalTimeoutDuration.has_value() ? Version::ANDROID_OC_MR1 : Version::ANDROID_R;
}

Result<Version> validateCacheToken(const CacheToken& cacheToken) {
    // A CacheToken of 0 is the default value, so it is implicitly valid for all versions.
    constexpr auto kDefaultCacheToken = CacheToken{};
    return cacheToken == kDefaultCacheToken ? Version::ANDROID_OC_MR1 : Version::ANDROID_Q;
}

Result<Version> validateSyncFence(const SyncFence& syncFence) {
    // The absence of a sync fence is implicitly valid for all versions.
    if (!syncFence.hasFd()) {
        return Version::ANDROID_OC_MR1;
    }
    NN_VALIDATE_GE(syncFence.getFd(), 0);
    return Version::ANDROID_R;
}

Result<Version> validateRequestArgumentsForModel(
        const std::vector<Request::Argument>& requestArguments,
        const std::vector<uint32_t>& operandIndexes, const std::vector<Operand>& operands,
        bool isOutput, bool allowUnspecifiedOutput) {
    auto version = Version::ANDROID_OC_MR1;
    // The request should specify as many arguments as were described in the model.
    const std::string_view type = isOutput ? "output" : "input";
    const size_t requestArgumentCount = requestArguments.size();
    NN_VALIDATE_EQ(requestArgumentCount, operandIndexes.size())
            << "Request specifies " << requestArgumentCount << " " << type << "s but the model has "
            << operandIndexes.size();
    for (size_t requestArgumentIndex = 0; requestArgumentIndex < requestArgumentCount;
         requestArgumentIndex++) {
        const Request::Argument& requestArgument = requestArguments[requestArgumentIndex];
        // Get the operand index for this argument. We extract it from the list
        // that was provided in the call to ANeuralNetworksModel_identifyInputsAndOutputs.
        // We assume in this function that the model has been validated already.
        const uint32_t operandIndex = operandIndexes[requestArgumentIndex];
        const Operand& operand = operands[operandIndex];
        if (requestArgument.lifetime != Request::Argument::LifeTime::NO_VALUE) {
            const bool isExtensionType = isExtension(operand.type);
            // If the argument specified a dimension, validate it.
            uint32_t modelRank = operand.dimensions.size();
            uint32_t requestRank = requestArgument.dimensions.size();
            if (requestRank == 0) {
                // NOTE: validateRequestArguments cannot validate unknown tensor rank with
                // extension operand type.
                if (!isExtensionType && !isNonExtensionScalar(operand.type)) {
                    if (modelRank <= 0) {
                        NN_VALIDATE(isOutput)
                                << "Model has unknown input rank but the request does not "
                                   "specify the rank.";
                        NN_VALIDATE(allowUnspecifiedOutput)
                                << "Model has unknown output rank and request does not specify it.";
                        // Unspecified output dimensions introduced in Android Q.
                        version = combineVersions(version, Version::ANDROID_Q);
                    }
                }
                // Validate that all the dimensions are specified in the model.
                for (size_t i = 0; i < modelRank; i++) {
                    if (operand.dimensions[i] == 0) {
                        NN_VALIDATE(isOutput && allowUnspecifiedOutput)
                                << "Model has dimension " << i
                                << " set to 0 but the request does not specify the dimension.";
                        // Unspecified output dimensions introduced in Android Q.
                        version = combineVersions(version, Version::ANDROID_Q);
                    }
                }
            } else {
                NN_VALIDATE(modelRank == 0 || requestRank == modelRank)
                        << "Request " << type << " " << requestArgumentIndex
                        << " has number of dimensions (" << requestRank
                        << ") different than the model's (" << modelRank << ")";
                for (size_t i = 0; i < requestRank; i++) {
                    NN_VALIDATE(modelRank == 0 || operand.dimensions[i] == 0 ||
                                requestArgument.dimensions[i] == operand.dimensions[i])
                            << "Request " << type << " " << requestArgumentIndex
                            << " has dimension " << i << " of " << requestArgument.dimensions[i]
                            << " different than the model's " << operand.dimensions[i];
                    if (requestArgument.dimensions[i] == 0) {
                        NN_VALIDATE(isOutput && allowUnspecifiedOutput)
                                << "Request " << type << " " << requestArgumentIndex
                                << " has dimension " << i << " of zero";
                        // Unspecified output dimensions introduced in Android Q.
                        version = combineVersions(version, Version::ANDROID_Q);
                    }
                }
            }
            // NOTE: validateRequestArguments cannot validate DataLocation::length
            // with extension operand type.
            if (!isExtensionType && requestArgument.location.length != 0) {
                const auto dimensions =
                        NN_TRY(combineDimensions(operand.dimensions, requestArgument.dimensions));
                const size_t expectedLength = getNonExtensionSize(operand.type, dimensions).value();
                if (expectedLength != 0) {
                    NN_VALIDATE_EQ(requestArgument.location.length, expectedLength)
                            << "Request " << type << " " << requestArgumentIndex
                            << " expected a size of " << expectedLength << " but got "
                            << requestArgument.location.length;
                }
            }
        }
    }
    return version;
}

Result<Version> validateRequestForModelImpl(const Request& request, const Model& model,
                                            bool allowUnspecifiedOutput) {
    auto version = NN_TRY(validateRequest(request));
    version = combineVersions(version, NN_TRY(validateModel(model)));
    version = combineVersions(version,
                              NN_TRY(validateRequestArgumentsForModel(
                                      request.inputs, model.main.inputIndexes, model.main.operands,
                                      /*isOutput=*/false, /*allowUnspecifiedOutput=*/true)));
    version = combineVersions(
            version, NN_TRY(validateRequestArgumentsForModel(
                             request.outputs, model.main.outputIndexes, model.main.operands,
                             /*isOutput=*/true, allowUnspecifiedOutput)));
    return version;
}

Result<Version> validateMemoryDescImpl(
        const BufferDesc& desc, const std::vector<SharedPreparedModel>& preparedModels,
        const std::vector<BufferRole>& inputRoles, const std::vector<BufferRole>& outputRoles,
        const std::function<const Model*(const SharedPreparedModel&)>& getModel,
        std::set<PreparedModelRole>* preparedModelRoles, Operand* combinedOperand) {
    NN_VALIDATE(!preparedModels.empty());
    NN_VALIDATE(!inputRoles.empty() || !outputRoles.empty());

    std::set<PreparedModelRole> roles;
    std::vector<nn::Operand> operands;
    operands.reserve(inputRoles.size() + outputRoles.size());
    for (const auto& role : inputRoles) {
        NN_VALIDATE_LT(role.modelIndex, preparedModels.size());
        const auto& preparedModel = preparedModels[role.modelIndex];
        NN_VALIDATE(preparedModel != nullptr);
        const auto* model = getModel(preparedModel);
        NN_VALIDATE(model != nullptr);
        const auto& inputIndexes = model->main.inputIndexes;
        NN_VALIDATE_LT(role.ioIndex, inputIndexes.size());
        NN_VALIDATE_GT(role.probability, 0.0f);
        NN_VALIDATE_LE(role.probability, 1.0f);
        const auto [it, success] = roles.emplace(preparedModel.get(), IOType::INPUT, role.ioIndex);
        NN_VALIDATE(success);
        operands.push_back(model->main.operands[inputIndexes[role.ioIndex]]);
    }
    for (const auto& role : outputRoles) {
        NN_VALIDATE_LT(role.modelIndex, preparedModels.size());
        const auto& preparedModel = preparedModels[role.modelIndex];
        NN_VALIDATE(preparedModel != nullptr);
        const auto* model = getModel(preparedModel);
        NN_VALIDATE(model != nullptr);
        const auto& outputIndexes = model->main.outputIndexes;
        NN_VALIDATE_LT(role.ioIndex, outputIndexes.size());
        NN_VALIDATE_GT(role.probability, 0.0f);
        NN_VALIDATE_LE(role.probability, 1.0f);
        const auto [it, success] = roles.emplace(preparedModel.get(), IOType::OUTPUT, role.ioIndex);
        NN_VALIDATE(success);
        operands.push_back(model->main.operands[outputIndexes[role.ioIndex]]);
    }

    CHECK(!operands.empty());
    const auto opType = operands.front().type;

    Dimensions dimensions = desc.dimensions;
    for (const auto& operand : operands) {
        NN_VALIDATE_EQ(operand.type, opType) << operand.type << " vs " << operands.front().type;
        NN_VALIDATE_EQ(operand.scale, operands.front().scale);
        NN_VALIDATE_EQ(operand.zeroPoint, operands.front().zeroPoint);
        // NOTE: validateMemoryDesc cannot validate extra parameters for extension operand type.
        if (!isExtension(opType)) {
            NN_VALIDATE_EQ(operand.extraParams, operands.front().extraParams)
                    << operand.extraParams << " vs " << operands.front().extraParams;
        }
        dimensions = NN_TRY(combineDimensions(dimensions, operand.dimensions));
    }

    // NOTE: validateMemoryDesc cannot validate scalar dimensions with extension operand type.
    if (!isExtension(opType)) {
        NN_VALIDATE(!isNonExtensionScalar(opType) || dimensions.empty())
                << "invalid dimensions with scalar operand type.";
    }

    if (preparedModelRoles != nullptr) {
        *preparedModelRoles = std::move(roles);
    }
    if (combinedOperand != nullptr) {
        *combinedOperand = operands.front();
        combinedOperand->dimensions = dimensions;
    }
    return Version::ANDROID_R;
}

class OperationValidationContext : public IOperationValidationContext {
    DISALLOW_IMPLICIT_CONSTRUCTORS(OperationValidationContext);

   public:
    OperationValidationContext(const char* operationName, const std::vector<uint32_t>& inputIndexes,
                               const std::vector<uint32_t>& outputIndexes,
                               const std::vector<Operand>& operands)
        : operationName(operationName),
          inputIndexes(inputIndexes),
          outputIndexes(outputIndexes),
          operands(operands) {}

    const char* getOperationName() const override;

    uint32_t getNumInputs() const override;
    OperandType getInputType(uint32_t index) const override;
    Shape getInputShape(uint32_t index) const override;
    const Operand::ExtraParams& getInputExtraParams(uint32_t index) const override;

    uint32_t getNumOutputs() const override;
    OperandType getOutputType(uint32_t index) const override;
    Shape getOutputShape(uint32_t index) const override;

   private:
    const Operand* getInputOperand(uint32_t index) const;
    const Operand* getOutputOperand(uint32_t index) const;

    const char* operationName;
    const std::vector<uint32_t>& inputIndexes;
    const std::vector<uint32_t>& outputIndexes;
    const std::vector<Operand>& operands;
};

const char* OperationValidationContext::getOperationName() const {
    return operationName;
}

const Operand* OperationValidationContext::getInputOperand(uint32_t index) const {
    return &operands.at(inputIndexes.at(index));
}

const Operand* OperationValidationContext::getOutputOperand(uint32_t index) const {
    return &operands.at(outputIndexes.at(index));
}

uint32_t OperationValidationContext::getNumInputs() const {
    auto count = inputIndexes.size();
    CHECK_LE(count, std::numeric_limits<uint32_t>::max());
    return static_cast<uint32_t>(count);
}

uint32_t OperationValidationContext::getNumOutputs() const {
    auto count = outputIndexes.size();
    CHECK_LE(count, std::numeric_limits<uint32_t>::max());
    return static_cast<uint32_t>(count);
}

OperandType OperationValidationContext::getInputType(uint32_t index) const {
    return getInputOperand(index)->type;
}

Shape OperationValidationContext::getInputShape(uint32_t index) const {
    const Operand* operand = getInputOperand(index);
    return {operand->type, operand->dimensions, operand->scale, operand->zeroPoint,
            operand->extraParams};
}

const Operand::ExtraParams& OperationValidationContext::getInputExtraParams(uint32_t index) const {
    return getInputOperand(index)->extraParams;
}

OperandType OperationValidationContext::getOutputType(uint32_t index) const {
    return getOutputOperand(index)->type;
}

Shape OperationValidationContext::getOutputShape(uint32_t index) const {
    const Operand* operand = getOutputOperand(index);
    return {operand->type, operand->dimensions, operand->scale, operand->zeroPoint,
            operand->extraParams};
}

// TODO(b/169345292): reduce the duplicate validation here

Result<void> validateOperandSymmPerChannelQuantParamsImpl(
        const Operand& operand, const Operand::SymmPerChannelQuantParams& channelQuant,
        const char* tag) {
    if (operand.type != OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
        NN_VALIDATE_FAIL();
    }

    NN_VALIDATE_LT(channelQuant.channelDim, operand.dimensions.size()) << tag;
    NN_VALIDATE(!channelQuant.scales.empty()) << tag;
    NN_VALIDATE_EQ(channelQuant.scales.size(), operand.dimensions[channelQuant.channelDim]) << tag;
    NN_VALIDATE_NE(operand.dimensions[channelQuant.channelDim], 0u)
            << tag << " channel dimension " << channelQuant.channelDim << " is underspecified";
    for (uint32_t i = 0; i < operand.dimensions[channelQuant.channelDim]; i++) {
        NN_VALIDATE_GT(channelQuant.scales[i], 0.0f) << tag << " invalid scaleArray[" << i << "]";
    }
    return {};
}

Result<void> validateScalarDimensions(const Operand& type, const char* tag) {
    NN_VALIDATE(type.dimensions.empty()) << tag << " invalid dimensions for scalar type";
    return {};
}

Result<void> validateQuant8AsymmParams(const Operand& type, const char* tag) {
    NN_VALIDATE(0 <= type.zeroPoint && type.zeroPoint <= 255)
            << tag << " invalid zeroPoint: " << type.zeroPoint;
    NN_VALIDATE_GT(type.scale, 0.0f) << tag << " invalid scale";
    return {};
}

Result<void> validateQuant8AsymmSignedParams(const Operand& type, const char* tag) {
    NN_VALIDATE(-128 <= type.zeroPoint && type.zeroPoint <= 127)
            << tag << " invalid zeroPoint: " << type.zeroPoint;
    NN_VALIDATE_GT(type.scale, 0.0f) << tag << " invalid scale";
    return {};
}

Result<void> validateQuant8SymmParams(const Operand& type, const char* tag) {
    NN_VALIDATE_EQ(type.zeroPoint, 0) << tag << " invalid zeroPoint: " << type.zeroPoint;
    NN_VALIDATE_GT(type.scale, 0.0f) << tag << " invalid scale";
    return {};
}

Result<void> validateQuant16AsymmParams(const Operand& type, const char* tag) {
    NN_VALIDATE(0 <= type.zeroPoint && type.zeroPoint <= 65535)
            << tag << " invalid zeroPoint: " << type.zeroPoint;
    NN_VALIDATE_GT(type.scale, 0.0f) << tag << " invalid scale";
    return {};
}

Result<void> validateQuantSymmParams(const Operand& type, const char* tag) {
    NN_VALIDATE_EQ(type.zeroPoint, 0) << tag << " zeroPoint is not zero";
    NN_VALIDATE_GT(type.scale, 0.0f) << tag << " invalid scale";
    return {};
}

Result<void> validateNoQuantParams(const Operand& type, const char* tag) {
    NN_VALIDATE_EQ(type.zeroPoint, 0) << tag << " zeroPoint is not zero";
    NN_VALIDATE_EQ(type.scale, 0.0f) << tag << " scale is not zero";
    return {};
}

Result<void> validateTensorDimensions(
        const Operand& type, const Extension::OperandTypeInformation* extensionOperandTypeInfo,
        const char* tag, bool allowPartial) {
    if (!allowPartial) {
        NN_VALIDATE(!type.dimensions.empty()) << tag << " invalid operand dimensions";
    }
    uint64_t size = isExtension(type.type) ? extensionOperandTypeInfo->byteSize
                                           : getNonExtensionSize(type.type);
    constexpr uint64_t kMaxSize = std::numeric_limits<uint32_t>::max();
    for (size_t i = 0; i < type.dimensions.size(); i++) {
        if (!allowPartial) {
            NN_VALIDATE_NE(type.dimensions[i], 0u) << tag << " invalid operand dimensions";
        }
        if (type.dimensions[i] != 0) {
            size *= type.dimensions[i];
            NN_VALIDATE_LE(size, kMaxSize) << tag << " operand byte size exceeds " << kMaxSize;
        }
    }
    return {};
}

Result<void> validateOperandTypeImpl(
        const Operand& type,
        const Extension::OperandTypeInformation* const extensionOperandTypeInfo, const char* tag,
        bool allowPartial) {
    if (isExtension(type.type)) {
        NN_VALIDATE(extensionOperandTypeInfo != nullptr);
        if (extensionOperandTypeInfo->isTensor) {
            NN_TRY(validateTensorDimensions(type, extensionOperandTypeInfo, tag, allowPartial));
        } else {
            NN_TRY(validateScalarDimensions(type, tag));
        }
        return validateNoQuantParams(type, tag);
    }

    NN_VALIDATE(extensionOperandTypeInfo == nullptr);
    NN_TRY(validateOperandType(type.type));

    if (isNonExtensionScalar(type.type)) {
        NN_TRY(validateScalarDimensions(type, tag));
        if (type.type != OperandType::OEM) {  // Historically, we have allowed OEM types
                                              // to use quantization parameters.
            NN_TRY(validateNoQuantParams(type, tag));
        }
    } else {
        NN_TRY(validateTensorDimensions(type, extensionOperandTypeInfo, tag, allowPartial));
        if (type.type == OperandType::TENSOR_QUANT8_ASYMM) {
            NN_TRY(validateQuant8AsymmParams(type, tag));
        } else if (type.type == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
            NN_TRY(validateQuant8AsymmSignedParams(type, tag));
        } else if (type.type == OperandType::TENSOR_QUANT8_SYMM) {
            NN_TRY(validateQuant8SymmParams(type, tag));
        } else if (type.type == OperandType::TENSOR_QUANT16_ASYMM) {
            NN_TRY(validateQuant16AsymmParams(type, tag));
        } else if (type.type == OperandType::TENSOR_QUANT16_SYMM) {
            NN_TRY(validateQuantSymmParams(type, tag));
        } else if (type.type == OperandType::TENSOR_INT32 ||
                   type.type == OperandType::TENSOR_OEM_BYTE) {
            // TODO(b/119869082): TENSOR_INT32 should not use quantization parameters.
            // Historically, we have allowed OEM types to use quantization parameters.
        } else {
            NN_TRY(validateNoQuantParams(type, tag));
        }
    }

    return {};
}

Result<void> validateOperandListImpl(const std::vector<uint32_t>& list, size_t operandCount,
                                     const char* tag) {
    for (size_t i = 0; i < list.size(); i++) {
        NN_VALIDATE_LT(list[i], operandCount) << tag << " invalid operand index at " << i << " = "
                                              << list[i] << ", operandCount " << operandCount;
    }
    return {};
}

Result<void> validateOperationOperandTypes(const std::vector<Operand>& operands,
                                           const std::vector<uint32_t>& inputIndexes,
                                           const std::vector<OperandType>& inExpectedTypes,
                                           const std::vector<uint32_t>& outputIndexes,
                                           const std::vector<OperandType>& outExpectedInTypes) {
    NN_VALIDATE_EQ(inputIndexes.size(), inExpectedTypes.size())
            << "Wrong operand count: expected " << inputIndexes.size() << " inputs, got "
            << inputIndexes.size() << " inputs";
    NN_VALIDATE_EQ(outputIndexes.size(), outExpectedInTypes.size())
            << "Wrong operand count: expected " << outputIndexes.size() << " outputs, got "
            << outputIndexes.size() << " outputs";
    for (size_t i = 0; i < inputIndexes.size(); i++) {
        NN_VALIDATE_EQ(operands[inputIndexes[i]].type, inExpectedTypes[i])
                << "Invalid input tensor type " << operands[inputIndexes[i]].type << " for input "
                << i << ", expected " << inExpectedTypes[i];
    }
    for (size_t i = 0; i < outputIndexes.size(); i++) {
        NN_VALIDATE_EQ(operands[outputIndexes[i]].type, outExpectedInTypes[i])
                << "Invalid output tensor type " << operands[outputIndexes[i]].type << " for input "
                << i << ", expected " << outExpectedInTypes[i];
    }

    return {};
}

Result<void> validateSubgraphReference(const std::vector<Model::Subgraph>& subgraphs,
                                       const Operand& modelOperand) {
    NN_VALIDATE_EQ(modelOperand.type, OperandType::SUBGRAPH)
            << "Unexpected operand type: " << modelOperand.type;
    NN_VALIDATE_LT(modelOperand.location.offset, subgraphs.size()) << "Invalid subgraph reference";
    return {};
}
const Model::Subgraph& getSubgraph(const std::vector<Model::Subgraph>& subgraphs,
                                   const Operand& modelOperand) {
    return subgraphs.at(modelOperand.location.offset);
}
uint32_t getInputCount(const std::vector<Model::Subgraph>& subgraphs, const Operand& modelOperand) {
    return getSubgraph(subgraphs, modelOperand).inputIndexes.size();
}
uint32_t getOutputCount(const std::vector<Model::Subgraph>& subgraphs,
                        const Operand& modelOperand) {
    return getSubgraph(subgraphs, modelOperand).outputIndexes.size();
}
const Operand& getInputOperand(const std::vector<Model::Subgraph>& subgraphs,
                               const Operand& modelOperand, uint32_t index) {
    const Model::Subgraph& subgraph = getSubgraph(subgraphs, modelOperand);
    return subgraph.operands.at(subgraph.inputIndexes.at(index));
}
const Operand& getOutputOperand(const std::vector<Model::Subgraph>& subgraphs,
                                const Operand& modelOperand, uint32_t index) {
    const Model::Subgraph& subgraph = getSubgraph(subgraphs, modelOperand);
    return subgraph.operands.at(subgraph.outputIndexes.at(index));
}

// Checks if two operands have the same types, ranks (if specified), dimensions
// (if specified), scales, zeroPoints, and extraParams.
Result<void> compatible(const Operand& a, const Operand& b) {
    NN_VALIDATE_EQ(a.type, b.type) << a.type << " != " << b.type;
    if (!a.dimensions.empty() && !b.dimensions.empty()) {
        NN_VALIDATE_EQ(a.dimensions.size(), b.dimensions.size()) << "Incompatible dimensions";
        for (uint32_t i = 0, n = a.dimensions.size(); i < n; ++i) {
            if (a.dimensions[i] != 0 && b.dimensions[i] != 0) {
                NN_VALIDATE_EQ(a.dimensions[i], b.dimensions[i]) << "Incompatible dimensions";
            }
        }
    }
    NN_VALIDATE_EQ(a.scale, b.scale);
    NN_VALIDATE_EQ(a.zeroPoint, b.zeroPoint);
    NN_VALIDATE_EQ(a.extraParams, b.extraParams) << a.extraParams << " != " << b.extraParams;
    return {};
}

Result<void> validateConditionOperand(const Operand& operand) {
    NN_VALIDATE_EQ(operand.type, OperandType::TENSOR_BOOL8)
            << "Unexpected condition operand type: " << operand.type;
    NN_VALIDATE_EQ(operand.dimensions.size(), 1u) << "Condition operand must be a singleton";
    NN_VALIDATE_EQ(operand.dimensions[0], 1u) << "Condition operand must be a singleton";
    return {};
}

Result<Version> validateIfOperation(const std::vector<uint32_t>& inputs,
                                    const std::vector<uint32_t>& outputs,
                                    const std::vector<Operand>& operands,
                                    const std::vector<Model::Subgraph>& subgraphs) {
    namespace op = operation_if;
    NN_VALIDATE_GE(inputs.size(), 3u) << "IF must have at least 3 inputs";
    NN_VALIDATE_GE(outputs.size(), 1u) << "IF must have at least 1 output";
    auto validateBranchOperand = [&](const Operand& branchModelOperand) -> Result<void> {
        auto result = validateSubgraphReference(subgraphs, branchModelOperand);
        if (!result.has_value()) {
            return error() << std::move(result).error()
                           << " -- Operand is not a valid subgraph reference";
        }
        const uint32_t branchModelInputCount = getInputCount(subgraphs, branchModelOperand);
        const uint32_t branchModelOutputCount = getOutputCount(subgraphs, branchModelOperand);
        NN_VALIDATE_EQ(inputs.size(), op::kFirstInput + branchModelInputCount);
        NN_VALIDATE_EQ(outputs.size(), branchModelOutputCount);
        for (uint32_t i = 0; i < branchModelInputCount; ++i) {
            const Operand& innerOperand = getInputOperand(subgraphs, branchModelOperand, i);
            const Operand& outerOperand = operands[inputs[op::kFirstInput + i]];
            NN_TRY(compatible(innerOperand, outerOperand));
        }
        for (uint32_t i = 0; i < branchModelOutputCount; ++i) {
            const Operand& innerOperand = getOutputOperand(subgraphs, branchModelOperand, i);
            const Operand& outerOperand = operands[outputs[i]];
            NN_TRY(compatible(innerOperand, outerOperand));
        }
        return {};
    };
    auto result = validateConditionOperand(operands[inputs[op::kCondBoolOperand]]);
    if (!result.has_value()) {
        return error() << std::move(result).error() << " for IF condition operand";
    }
    result = validateBranchOperand(operands[inputs[op::kThenModelOperand]]);
    if (!result.has_value()) {
        return error() << std::move(result).error() << " for IF then model";
    }
    result = validateBranchOperand(operands[inputs[op::kElseModelOperand]]);
    if (!result.has_value()) {
        return error() << std::move(result).error() << " for IF else model";
    }
    return Version::ANDROID_R;
}

Result<Version> validateControlFlowOperandUnknownSize(const Operand& operand) {
    if (!isExtension(operand.type) && getNonExtensionSize(operand).value() == 0) {
        // 1.3 HAL (corresponding to Version::ANDROID_R) does not support CF operations with
        // operands of unknown size. See http://b/132458982#comment63.
        return Version::CURRENT_RUNTIME;
    }
    return Version::ANDROID_R;
}

Result<Version> validateWhileOperation(const std::vector<uint32_t>& inputs,
                                       const std::vector<uint32_t>& outputs,
                                       const std::vector<Operand>& operands,
                                       const std::vector<Model::Subgraph>& subgraphs) {
    // Let the loop have
    // - m >= 1 input-output operands,
    // - k >= 0 state-only operands, and
    // - n >= 0 input-only operands.
    // Then
    // - the WHILE loop operation has (2 + m + k + n) inputs and m outputs.
    // - the condition model has (m + k + n) inputs and 1 output.
    // - the body model has (m + k + n) inputs and (m + k) outputs.
    namespace op = operation_while;
    NN_VALIDATE_GE(inputs.size(), 3u) << "WHILE must have at least 3 inputs";
    NN_VALIDATE_GE(outputs.size(), 1u) << "WHILE must have at least 1 output";
    auto validateCondOperand = [&](const Operand& condModelOperand) -> Result<Version> {
        Version version = Version::ANDROID_R;
        auto result = validateSubgraphReference(subgraphs, condModelOperand);
        if (!result.has_value()) {
            return error() << std::move(result).error()
                           << " -- Operand is not a valid subgraph reference";
        }
        const uint32_t condModelInputCount = getInputCount(subgraphs, condModelOperand);
        const uint32_t condModelOutputCount = getOutputCount(subgraphs, condModelOperand);
        NN_VALIDATE_EQ(inputs.size(), op::kFirstInput + condModelInputCount);
        NN_VALIDATE_EQ(condModelOutputCount, 1u);
        for (uint32_t i = 0; i < condModelInputCount; ++i) {
            const Operand& innerOperand = getInputOperand(subgraphs, condModelOperand, i);
            const Operand& outerOperand = operands[inputs[op::kFirstInput + i]];
            NN_TRY(compatible(innerOperand, outerOperand));
            version = combineVersions(version,
                                      NN_TRY(validateControlFlowOperandUnknownSize(innerOperand)));
            version = combineVersions(version,
                                      NN_TRY(validateControlFlowOperandUnknownSize(outerOperand)));
        }
        NN_TRY(validateConditionOperand(getOutputOperand(subgraphs, condModelOperand, 0)));
        return version;
    };
    auto validateBodyOperand = [&](const Operand& bodyModelOperand) -> Result<Version> {
        Version version = Version::ANDROID_R;
        auto result = validateSubgraphReference(subgraphs, bodyModelOperand);
        if (!result.has_value()) {
            return error() << std::move(result).error()
                           << " -- Operand is not a valid subgraph reference";
        }
        const uint32_t bodyModelInputCount = getInputCount(subgraphs, bodyModelOperand);
        const uint32_t bodyModelOutputCount = getOutputCount(subgraphs, bodyModelOperand);
        NN_VALIDATE_EQ(inputs.size(), op::kFirstInput + bodyModelInputCount);
        NN_VALIDATE_GE(bodyModelOutputCount, outputs.size());
        NN_VALIDATE_GE(bodyModelInputCount, bodyModelOutputCount);
        const uint32_t inputOutputCount = outputs.size();
        const uint32_t stateOnlyCount = bodyModelOutputCount - inputOutputCount;
        const uint32_t inputOnlyCount = bodyModelInputCount - bodyModelOutputCount;
        for (uint32_t i = 0, n = inputOutputCount + stateOnlyCount + inputOnlyCount; i < n; ++i) {
            const Operand& innerOperand = getInputOperand(subgraphs, bodyModelOperand, i);
            const Operand& outerOperand = operands[inputs[op::kFirstInput + i]];
            NN_TRY(compatible(innerOperand, outerOperand));
            version = combineVersions(version,
                                      NN_TRY(validateControlFlowOperandUnknownSize(innerOperand)));
            version = combineVersions(version,
                                      NN_TRY(validateControlFlowOperandUnknownSize(outerOperand)));
        }
        for (uint32_t i = 0; i < inputOutputCount; ++i) {
            const Operand& innerOperand = getOutputOperand(subgraphs, bodyModelOperand, i);
            const Operand& outerOperand = operands[outputs[i]];
            NN_TRY(compatible(innerOperand, outerOperand));
            version = combineVersions(version,
                                      NN_TRY(validateControlFlowOperandUnknownSize(outerOperand)));
        }
        for (uint32_t i = 0, n = inputOutputCount + stateOnlyCount; i < n; ++i) {
            const Operand& inputOperand = getInputOperand(subgraphs, bodyModelOperand, i);
            const Operand& outputOperand = getOutputOperand(subgraphs, bodyModelOperand, i);
            NN_TRY(compatible(inputOperand, outputOperand));
            version = combineVersions(version,
                                      NN_TRY(validateControlFlowOperandUnknownSize(outputOperand)));
        }
        return version;
    };
    auto result = validateCondOperand(operands[inputs[op::kCondModelOperand]]);
    if (!result.has_value()) {
        return error() << std::move(result).error() << " for WHILE condition model";
    }
    auto version = result.value();
    result = validateBodyOperand(operands[inputs[op::kBodyModelOperand]]);
    if (!result.has_value()) {
        return error() << std::move(result).error() << " for WHILE body model";
    }
    version = combineVersions(version, result.value());
    return version;
}

Result<Version> validateOperationButNotOperandsImpl(const Operation& operation,
                                                    const std::vector<Operand>& operands,
                                                    const std::vector<Model::Subgraph>& subgraphs) {
    const auto opType = operation.type;
    const auto& inputIndexes = operation.inputs;
    const auto& outputIndexes = operation.outputs;

    NN_TRY(validateOperandListImpl(inputIndexes, operands.size(),
                                   "ANeuralNetworksModel_addOperation inputs"));
    NN_TRY(validateOperandListImpl(outputIndexes, operands.size(),
                                   "ANeuralNetworksModel_addOperation outputs"));

    if (isExtension(opType)) {
        // There is no other validation we can do for an extension operation.
        return Version::ANDROID_Q;
    }

    auto invalidInOutNumberMessage = [opType, &inputIndexes, &outputIndexes](int expIn,
                                                                             int expOut) {
        std::ostringstream os;
        os << "Invalid number of input operands (" << inputIndexes.size() << ", expected " << expIn
           << ") or output operands (" << outputIndexes.size() << ", expected " << expOut
           << ") for operation " << opType;
        return os.str();
    };

    switch (opType) {
        case OperationType::OEM_OPERATION: {
            return Version::ANDROID_OC_MR1;
        }
        case OperationType::RESHAPE: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {OperandType::TENSOR_FLOAT16, OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            const auto inputRank = operands[inputIndexes[0]].dimensions.size();
            NN_VALIDATE_LE(inputRank, 4u)
                    << "Unsupported input tensor rank for operation " << opType;
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::DEPTH_TO_SPACE: {
            NN_VALIDATE((inputIndexes.size() == 3 || inputIndexes.size() == 2) &&
                        outputIndexes.size() == 1)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 3 or 2) or output operands (" << outputIndexes.size()
                    << ", expected 1) for operation " << opType;
            auto inputType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {OperandType::TENSOR_FLOAT16, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            if (inputIndexes.size() == 3) {
                inExpectedTypes.push_back(OperandType::BOOL);
                version = combineVersions(version, Version::ANDROID_Q);
            } else {
                version = combineVersions(version, Version::ANDROID_OC_MR1);
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::SPACE_TO_DEPTH: {
            NN_VALIDATE((inputIndexes.size() == 3 || inputIndexes.size() == 2) &&
                        outputIndexes.size() == 1)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 3 or 2) or output operands (" << outputIndexes.size()
                    << ", expected 1) for operation " << opType;
            auto inputType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {OperandType::TENSOR_FLOAT16, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            if (inputIndexes.size() == 3) {
                inExpectedTypes.push_back(OperandType::BOOL);
                version = combineVersions(version, Version::ANDROID_Q);
            } else {
                version = combineVersions(version, Version::ANDROID_OC_MR1);
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::EMBEDDING_LOOKUP: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[1]].type;
            NN_VALIDATE(inputType == OperandType::TENSOR_FLOAT16 ||
                        inputType == OperandType::TENSOR_FLOAT32 ||
                        inputType == OperandType::TENSOR_INT32 ||
                        inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                        inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED)
                    << "Unsupported input tensor type for operation " << opType;
            Version version;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_INT32, inputType};
            std::vector<OperandType> outExpectedTypes = {inputType};
            if (inputType == OperandType::TENSOR_FLOAT16 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else if (inputType == OperandType::TENSOR_INT32 ||
                       inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                version = Version::ANDROID_Q;
            } else {
                version = Version::ANDROID_OC_MR1;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::HASHTABLE_LOOKUP: {
            NN_VALIDATE(inputIndexes.size() == 3 && outputIndexes.size() == 2)
                    << invalidInOutNumberMessage(3, 2);
            auto inputType = operands[inputIndexes[2]].type;
            NN_VALIDATE(inputType == OperandType::TENSOR_FLOAT32 ||
                        inputType == OperandType::TENSOR_INT32 ||
                        inputType == OperandType::TENSOR_QUANT8_ASYMM)
                    << "Unsupported input tensor type for operation " << opType;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_INT32,
                                                        OperandType::TENSOR_INT32, inputType};
            std::vector<OperandType> outExpectedTypes = {inputType,
                                                         OperandType::TENSOR_QUANT8_ASYMM};
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return Version::ANDROID_OC_MR1;
        }
        case OperationType::LSH_PROJECTION: {
            NN_VALIDATE(inputIndexes.size() == 4 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(4, 1);
            auto inputType = operands[inputIndexes[1]].type;
            NN_VALIDATE(inputType == OperandType::TENSOR_FLOAT16 ||
                        inputType == OperandType::TENSOR_FLOAT32 ||
                        inputType == OperandType::TENSOR_INT32 ||
                        inputType == OperandType::TENSOR_QUANT8_ASYMM)
                    << "Unsupported input tensor type for operation " << opType;
            auto hashType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            if (hashType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT16,
                        inputType,
                        OperandType::TENSOR_FLOAT16,
                        OperandType::INT32,
                };
            } else if (hashType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT32,
                        inputType,
                        OperandType::TENSOR_FLOAT32,
                        OperandType::INT32,
                };
            } else {
                NN_VALIDATE_FAIL() << "Unsupported hash tensor type for operation " << opType;
            }
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_INT32};
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::BIDIRECTIONAL_SEQUENCE_LSTM: {
            const uint32_t kNumOutputs = 2;
            const uint32_t kNumOutputsMerged = 1;
            const uint32_t kNumOutputsWithState = 6;
            const uint32_t kNumOutputsMergedWithState = 5;
            NN_VALIDATE(inputIndexes.size() == 61 &&
                        (outputIndexes.size() == kNumOutputs ||
                         outputIndexes.size() == kNumOutputsMerged ||
                         outputIndexes.size() == kNumOutputsWithState ||
                         outputIndexes.size() == kNumOutputsMergedWithState))
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 61) or output operands (" << outputIndexes.size()
                    << ", expected 1, 2, 5 or 6) for operation " << opType;

            std::vector<OperandType> inExpectedTypes;
            auto inputType = operands[inputIndexes[0]].type;
            NN_VALIDATE(inputType == OperandType::TENSOR_FLOAT32 ||
                        inputType == OperandType::TENSOR_FLOAT16)
                    << "Unsupported input tensor type for operation " << opType;

            inExpectedTypes = {};
            for (int i = 0; i < 48; ++i) {
                inExpectedTypes.push_back(inputType);
            }
            inExpectedTypes.push_back(OperandType::INT32);
            inExpectedTypes.push_back(inputType == OperandType::TENSOR_FLOAT32
                                              ? OperandType::FLOAT32
                                              : OperandType::FLOAT16);
            inExpectedTypes.push_back(inputType == OperandType::TENSOR_FLOAT32
                                              ? OperandType::FLOAT32
                                              : OperandType::FLOAT16);
            inExpectedTypes.push_back(OperandType::BOOL);
            inExpectedTypes.push_back(OperandType::BOOL);
            for (int i = 0; i < 8; ++i) {
                inExpectedTypes.push_back(inputType);
            }

            Version version = Version::ANDROID_Q;
            if (outputIndexes.size() == kNumOutputsWithState ||
                outputIndexes.size() == kNumOutputsMergedWithState) {
                version = Version::ANDROID_R;
            }
            std::vector<OperandType> outExpectedTypes(outputIndexes.size(), inputType);
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::LSTM: {
            NN_VALIDATE((inputIndexes.size() == 23 || inputIndexes.size() == 27) &&
                        outputIndexes.size() == 4)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 23 or 27) or output operands (" << outputIndexes.size()
                    << ", expected 4) for operation " << opType;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            auto inputType = operands[inputIndexes[0]].type;
            NN_VALIDATE(inputType == OperandType::TENSOR_FLOAT32 ||
                        inputType == OperandType::TENSOR_FLOAT16)
                    << "Unsupported input tensor type for operation " << opType;

            Version version = Version::ANDROID_OC_MR1;
            inExpectedTypes = {inputType,         inputType, inputType, inputType, inputType,
                               inputType,         inputType, inputType, inputType, inputType,
                               inputType,         inputType, inputType, inputType, inputType,
                               inputType,         inputType, inputType, inputType, inputType,
                               OperandType::INT32};
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes.push_back(OperandType::FLOAT32);
                inExpectedTypes.push_back(OperandType::FLOAT32);
            } else {
                version = Version::ANDROID_Q;
                inExpectedTypes.push_back(OperandType::FLOAT16);
                inExpectedTypes.push_back(OperandType::FLOAT16);
            }

            outExpectedTypes = {inputType, inputType, inputType, inputType};
            if (inputIndexes.size() == 23) {
                version = combineVersions(version, Version::ANDROID_OC_MR1);
            } else {
                version = combineVersions(version, Version::ANDROID_Q);
                for (int i = 0; i < 4; ++i) {
                    inExpectedTypes.push_back(inputType);
                }
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::QUANTIZED_16BIT_LSTM: {
            NN_VALIDATE(inputIndexes.size() == 15 && outputIndexes.size() == 2)
                    << invalidInOutNumberMessage(15, 2);
            std::vector<OperandType> inExpectedTypes = {
                    OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_QUANT8_ASYMM,
                    OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_QUANT8_ASYMM,
                    OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_QUANT8_ASYMM,
                    OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_QUANT8_ASYMM,
                    OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_INT32,
                    OperandType::TENSOR_INT32,        OperandType::TENSOR_INT32,
                    OperandType::TENSOR_INT32,        OperandType::TENSOR_QUANT16_SYMM,
                    OperandType::TENSOR_QUANT8_ASYMM};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_QUANT16_SYMM,
                                                         OperandType::TENSOR_QUANT8_ASYMM};
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return Version::ANDROID_Q;
        }
        case OperationType::RANDOM_MULTINOMIAL: {
            NN_VALIDATE(inputIndexes.size() == 3 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(3, 1);
            OperandType inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_FLOAT16) {
                inExpectedTypes = {inputType, OperandType::INT32, OperandType::TENSOR_INT32};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_INT32};
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return Version::ANDROID_Q;
        }
        case OperationType::RNN: {
            NN_VALIDATE(inputIndexes.size() == 6 && outputIndexes.size() == 2)
                    << invalidInOutNumberMessage(6, 2);
            OperandType inputType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_OC_MR1;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_FLOAT32, OperandType::INT32,
                };
                outExpectedTypes = {
                        OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_FLOAT32,
                };
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_FLOAT16, OperandType::INT32,
                };
                outExpectedTypes = {
                        OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_FLOAT16,
                };
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::SVDF: {
            NN_VALIDATE(inputIndexes.size() == 7 && outputIndexes.size() == 2)
                    << invalidInOutNumberMessage(7, 2);
            Version version;
            OperandType inputType = operands[inputIndexes[0]].type;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_OC_MR1;
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            std::vector<OperandType> inExpectedTypes = {
                    inputType, inputType,          inputType,          inputType,
                    inputType, OperandType::INT32, OperandType::INT32,
            };
            std::vector<OperandType> outExpectedTypes = {inputType, inputType};
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::BATCH_TO_SPACE_ND: {
            NN_VALIDATE((inputIndexes.size() == 3 || inputIndexes.size() == 2) &&
                        outputIndexes.size() == 1)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 3 or 2) or output operands (" << outputIndexes.size()
                    << ", expected 1) for operation " << opType;
            auto inputType = operands[inputIndexes[0]].type;
            Version version = Version::ANDROID_OC_MR1;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {
                        OperandType::TENSOR_QUANT8_ASYMM,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
                inExpectedTypes = {
                        OperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            if (inputIndexes.size() == 3) {
                inExpectedTypes.push_back(OperandType::BOOL);
                version = combineVersions(version, Version::ANDROID_Q);
            } else {
                version = combineVersions(version, Version::ANDROID_P);
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::SPACE_TO_BATCH_ND: {
            NN_VALIDATE((inputIndexes.size() == 4 || inputIndexes.size() == 3) &&
                        outputIndexes.size() == 1)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 4 or 3) or output operands (" << outputIndexes.size()
                    << ", expected 1) for operation " << opType;
            auto inputType = operands[inputIndexes[0]].type;
            Version version = Version::ANDROID_OC_MR1;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_INT32,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_INT32,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                if (operands[inputIndexes[0]].zeroPoint != 0) {
                    version = Version::ANDROID_Q;
                }
                inExpectedTypes = {
                        OperandType::TENSOR_QUANT8_ASYMM,
                        OperandType::TENSOR_INT32,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
                inExpectedTypes = {
                        OperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                        OperandType::TENSOR_INT32,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM_SIGNED};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            if (inputIndexes.size() == 4) {
                inExpectedTypes.push_back(OperandType::BOOL);
                version = combineVersions(version, Version::ANDROID_Q);
            } else {
                version = combineVersions(version, Version::ANDROID_P);
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::PAD: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_P;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                       inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                    version = Version::ANDROID_R;
                } else {
                    if (operands[inputIndexes[0]].zeroPoint == 0) {
                        version = Version::ANDROID_P;
                    } else {
                        version = Version::ANDROID_Q;
                    }
                }
                inExpectedTypes = {
                        inputType,
                        OperandType::TENSOR_INT32,
                };
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            const auto inputRank = operands[inputIndexes[0]].dimensions.size();
            NN_VALIDATE_LE(inputRank, 4u)
                    << "Unsupported input tensor rank for operation " << opType;
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::PAD_V2: {
            NN_VALIDATE(inputIndexes.size() == 3 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(3, 1);
            auto inputType = operands[inputIndexes[0]].type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT32,
                        OperandType::TENSOR_INT32,
                        OperandType::FLOAT32,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {
                        OperandType::TENSOR_FLOAT16,
                        OperandType::TENSOR_INT32,
                        OperandType::FLOAT16,
                };
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                       inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                    version = Version::ANDROID_R;
                } else {
                    version = Version::ANDROID_Q;
                }
                inExpectedTypes = {
                        inputType,
                        OperandType::TENSOR_INT32,
                        OperandType::INT32,
                };  // TODO(b/116699425): Make it UINT8.
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            const auto inputRank = operands[inputIndexes[0]].dimensions.size();
            NN_VALIDATE_LE(inputRank, 4u)
                    << "Unsupported input tensor rank for operation " << opType;
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::CAST: {
            NN_VALIDATE(inputIndexes.size() == 1 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(1, 1);
            auto inputOperand = operands[inputIndexes[0]];
            auto outputOperand = operands[outputIndexes[0]];
            auto inputType = inputOperand.type;
            auto outputType = outputOperand.type;
            Version version;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if ((inputType == OperandType::TENSOR_FLOAT16 ||
                 inputType == OperandType::TENSOR_FLOAT32 ||
                 inputType == OperandType::TENSOR_INT32 ||
                 inputType == OperandType::TENSOR_QUANT8_ASYMM) &&
                (outputType == OperandType::TENSOR_FLOAT16 ||
                 outputType == OperandType::TENSOR_FLOAT32 ||
                 outputType == OperandType::TENSOR_INT32 ||
                 outputType == OperandType::TENSOR_QUANT8_ASYMM)) {
                version = Version::ANDROID_Q;
                inExpectedTypes = {inputType};
                outExpectedTypes = {outputType};
            } else if (inputType == OperandType::TENSOR_BOOL8 ||
                       inputType == OperandType::TENSOR_QUANT16_ASYMM ||
                       inputType == OperandType::TENSOR_QUANT16_SYMM ||
                       inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED ||
                       inputType == OperandType::TENSOR_QUANT8_SYMM) {
                version = Version::ANDROID_R;
                inExpectedTypes = {inputType};
                outExpectedTypes = {inputType};  // Only identity CAST is supported.
            } else {
                NN_VALIDATE_FAIL() << "Unsupported data type for operation " << opType;
            }
            // Validate that output shape is equal to input shape if dimensions
            // are already known.
            auto getNumberOfElements = [](const std::vector<uint32_t>& dims) {
                if (dims.empty()) {
                    return 0;
                }
                return std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<>());
            };
            NN_VALIDATE(inputOperand.dimensions.empty() || outputOperand.dimensions.empty() ||
                        getNumberOfElements(outputOperand.dimensions) == 0 ||
                        inputOperand.dimensions == outputOperand.dimensions);
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::MEAN: {
            NN_VALIDATE(inputIndexes.size() == 3 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(3, 1);
            const auto inputRank = operands[inputIndexes[0]].dimensions.size();
            NN_VALIDATE_LE(inputRank, 4u)
                    << "Unsupported input tensor rank for operation " << opType;
            auto inputType = operands[inputIndexes[0]].type;
            Version version;
            if (inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                version = Version::ANDROID_P;
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                version = Version::ANDROID_Q;
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            std::vector<OperandType> inExpectedTypes = {inputType, OperandType::TENSOR_INT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {inputType};
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::ARGMAX:
        case OperationType::ARGMIN: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT16 ||
                inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                inExpectedTypes = {inputType, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_INT32};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return Version::ANDROID_Q;
        }
        case OperationType::EXPAND_DIMS: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT16 ||
                inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                inExpectedTypes = {inputType, OperandType::INT32};
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            Version version;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                version = Version::ANDROID_Q;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::SPLIT: {
            NN_VALIDATE_EQ(inputIndexes.size(), 3u)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 3)" << opType;
            auto inputType = operands[inputIndexes[0]].type;
            NN_VALIDATE(inputType == OperandType::TENSOR_FLOAT16 ||
                        inputType == OperandType::TENSOR_FLOAT32 ||
                        inputType == OperandType::TENSOR_INT32 ||
                        inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                        inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED)
                    << "Unsupported input tensor type for operation " << opType;
            Version version;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                version = Version::ANDROID_Q;
            }
            std::vector<OperandType> inExpectedTypes = {inputType, OperandType::INT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes(outputIndexes.size(), inputType);
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::MAXIMUM:
        case OperationType::MINIMUM: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            OperandType inputType = operands[inputIndexes[0]].type;
            if (inputType == OperandType::TENSOR_FLOAT16 ||
                inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                inExpectedTypes = {inputType, inputType};
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            Version version;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                version = Version::ANDROID_Q;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::GROUPED_CONV_2D: {
            NN_VALIDATE((inputIndexes.size() == 12 || inputIndexes.size() == 9) &&
                        outputIndexes.size() == 1)
                    << "Invalid number of input operands (" << inputIndexes.size()
                    << ", expected 12 or 9) or output operands (" << outputIndexes.size()
                    << ", expected 1) for operation " << opType;
            auto inputType = operands[inputIndexes[0]].type;
            auto filterType = operands[inputIndexes[1]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32, OperandType::INT32,
                                   OperandType::INT32,          OperandType::INT32,
                                   OperandType::INT32,          OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_FLOAT16) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                                   OperandType::TENSOR_FLOAT16, OperandType::INT32,
                                   OperandType::INT32,          OperandType::INT32,
                                   OperandType::INT32,          OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT16};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                       inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                NN_VALIDATE(filterType == inputType ||
                            filterType == OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
                        << "Unsupported filter tensor type for operation " << opType;

                NN_VALIDATE(filterType != OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL ||
                            std::get<Operand::SymmPerChannelQuantParams>(
                                    operands[inputIndexes[1]].extraParams)
                                            .channelDim == 0)
                        << "Unsupported filter tensor channel dimension for operation " << opType;

                inExpectedTypes = {
                        inputType,          filterType,         OperandType::TENSOR_INT32,
                        OperandType::INT32, OperandType::INT32, OperandType::INT32,
                        OperandType::INT32, OperandType::INT32};
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }

            if (inputIndexes.size() == 12) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(), explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            inExpectedTypes.push_back(OperandType::BOOL);
            Version version;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                version = Version::ANDROID_Q;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::TILE: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT16 ||
                inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                inExpectedTypes = {inputType, OperandType::TENSOR_INT32};
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            Version version;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                version = Version::ANDROID_Q;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::POW: {
            NN_VALIDATE(inputIndexes.size() == 2 && outputIndexes.size() == 1)
                    << invalidInOutNumberMessage(2, 1);
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT16 ||
                inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {inputType, inputType};
                outExpectedTypes = {inputType};
            } else {
                NN_VALIDATE_FAIL() << "Unsupported input tensor type for operation " << opType;
            }
            Version version;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                version = Version::ANDROID_R;
            } else {
                version = Version::ANDROID_Q;
            }
            NN_TRY(validateOperationOperandTypes(operands, inputIndexes, inExpectedTypes,
                                                 outputIndexes, outExpectedTypes));
            return version;
        }
        case OperationType::IF: {
            return validateIfOperation(inputIndexes, outputIndexes, operands, subgraphs);
        }
        case OperationType::WHILE: {
            return validateWhileOperation(inputIndexes, outputIndexes, operands, subgraphs);
        }
        default: {
            const OperationRegistration* operationRegistration =
                    BuiltinOperationResolver::get()->findOperation(
                            static_cast<OperationType>(opType));
            // TODO: return ErrorStatus::UNEXPECTED_NULL
            NN_VALIDATE(operationRegistration != nullptr) << opType << " not registered";
            // TODO: return ErrorStatus::UNEXPECTED_NULL
            NN_VALIDATE(operationRegistration->validate != nullptr)
                    << "Incomplete operation registration: " << opType;

            OperationValidationContext context(operationRegistration->name, inputIndexes,
                                               outputIndexes, operands);
            return operationRegistration->validate(&context);
        }
    }
}

Result<Version> validateOperationIncludingOperandVersions(
        const Operation& operation, const std::vector<Operand>& operands,
        const std::vector<Version>& operandVersions,
        const std::vector<Model::Subgraph>& subgraphs) {
    auto version = NN_TRY(validateOperationButNotOperandsImpl(operation, operands, subgraphs));
    for (uint32_t index : operation.inputs) {
        version = combineVersions(version, operandVersions[index]);
    }
    for (uint32_t index : operation.outputs) {
        version = combineVersions(version, operandVersions[index]);
    }
    return version;
}

}  // anonymous namespace

// Below this point are all the functions that are declared in Validation.h. The convention of this
// file is to keep the function bodies of the functions declared in Validation.h minimal, meaning
// that most functions below simply redirect to one of the functions defined above in the anonymous
// namespace. If there is a function name clash between one of the functions below and one of the
// functions above, the function in the anonymous namespace is appended with "Impl".

Version combineVersions(Version lhs, Version rhs) {
    return std::max<Version>(lhs, rhs);
}

Result<Version> validate(const DeviceStatus& deviceStatus) {
    return validateDeviceStatus(deviceStatus);
}

Result<Version> validate(const ExecutionPreference& executionPreference) {
    return validateExecutionPreference(executionPreference);
}

Result<Version> validate(const DeviceType& deviceType) {
    return validateDeviceType(deviceType);
}

Result<Version> validate(const MeasureTiming& measureTiming) {
    return validateMeasureTiming(measureTiming);
}

Result<Version> validate(const OperandType& operandType) {
    return validateOperandType(operandType);
}

Result<Version> validate(const Priority& priority) {
    return validatePriority(priority);
}

Result<Version> validate(const ErrorStatus& errorStatus) {
    return validateErrorStatus(errorStatus);
}

Result<Version> validate(const FusedActivationFunc& activation) {
    return validateFusedActivationFunc(activation);
}

Result<Version> validate(const OutputShape& outputShape) {
    return validateOutputShape(outputShape);
}

Result<Version> validate(const Timing& timing) {
    return validateTiming(timing);
}

Result<Version> validate(const Capabilities& capabilities) {
    return validateCapabilities(capabilities);
}

Result<Version> validate(const Extension& extension) {
    return validateExtension(extension);
}

Result<Version> validate(const SharedHandle& handle) {
    return validateSharedHandle(handle);
}

Result<Version> validate(const SharedMemory& memory) {
    return validateSharedMemory(memory);
}

Result<Version> validate(const Model& model) {
    return validateModel(model);
}

Result<Version> validate(const BufferDesc& bufferDesc) {
    return validateBufferDesc(bufferDesc);
}

Result<Version> validate(const BufferRole& bufferRole) {
    return validateBufferRole(bufferRole);
}

Result<Version> validate(const Request& request) {
    return validateRequest(request);
}

Result<Version> validate(const OptionalTimePoint& optionalTimePoint) {
    return validateOptionalTimePoint(optionalTimePoint);
}

Result<Version> validate(const OptionalDuration& optionalTimeoutDuration) {
    return validateOptionalTimeoutDuration(optionalTimeoutDuration);
}

Result<Version> validate(const CacheToken& cacheToken) {
    return validateCacheToken(cacheToken);
}

Result<Version> validate(const SyncFence& syncFence) {
    return validateSyncFence(syncFence);
}

Result<Version> validate(const std::vector<OutputShape>& outputShapes) {
    return validateVector(outputShapes, validateOutputShape);
}

Result<Version> validate(const std::vector<Extension>& extensions) {
    return validateExtensions(extensions);
}

Result<Version> validate(const std::vector<SharedHandle>& handles) {
    return validateVector(handles, validateSharedHandle);
}

Result<Version> validate(const std::vector<BufferRole>& bufferRoles) {
    return validateVector(bufferRoles, validateBufferRole);
}

Result<Version> validate(const std::vector<SyncFence>& syncFences) {
    return validateVector(syncFences, validateSyncFence);
}

Result<Version> validateRequestForModel(const Request& request, const Model& model,
                                        bool allowUnspecifiedOutput) {
    return validateRequestForModelImpl(request, model, allowUnspecifiedOutput);
}

Result<Version> validateMemoryDesc(
        const BufferDesc& desc, const std::vector<SharedPreparedModel>& preparedModels,
        const std::vector<BufferRole>& inputRoles, const std::vector<BufferRole>& outputRoles,
        const std::function<const Model*(const SharedPreparedModel&)>& getModel,
        std::set<PreparedModelRole>* preparedModelRoles, Operand* combinedOperand) {
    return validateMemoryDescImpl(desc, preparedModels, inputRoles, outputRoles, getModel,
                                  preparedModelRoles, combinedOperand);
}

Result<void> validateOperandSymmPerChannelQuantParams(
        const Operand& operand, const Operand::SymmPerChannelQuantParams& channelQuant,
        const char* tag) {
    return validateOperandSymmPerChannelQuantParamsImpl(operand, channelQuant, tag);
}

Result<void> validateOperandType(const Operand& type,
                                 const Extension::OperandTypeInformation* extensionOperandTypeInfo,
                                 const char* tag, bool allowPartial) {
    return validateOperandTypeImpl(type, extensionOperandTypeInfo, tag, allowPartial);
}

Result<void> validateOperandList(const std::vector<uint32_t>& list, size_t operandCount,
                                 const char* tag) {
    return validateOperandListImpl(list, operandCount, tag);
}

Result<void> validateOperationButNotOperands(const Operation& operation,
                                             const std::vector<Operand>& operands,
                                             const std::vector<Model::Subgraph>& subgraphs) {
    NN_TRY(validateOperationButNotOperandsImpl(operation, operands, subgraphs));
    return {};
}

struct SubgraphVersionCache {
    std::vector<std::optional<Version>> cache;
};

std::unique_ptr<SubgraphVersionCache, void (*)(SubgraphVersionCache*)> createSubgraphVersionCache(
        size_t referencedSubgraphCount) {
    auto subgraphVersionCache = std::make_unique<SubgraphVersionCache>();
    subgraphVersionCache->cache.resize(referencedSubgraphCount);
    constexpr auto deleter = [](SubgraphVersionCache* ptr) { delete ptr; };
    return {subgraphVersionCache.release(), deleter};
}

Result<Version> validateOperationAndAnythingItDependsOn(
        const Operation& operation, const std::vector<Operand>& operands, size_t operandValuesSize,
        const std::vector<size_t>& poolSizes, const std::vector<Model::Subgraph>& subgraphs,
        SubgraphVersionCache* subgraphVersionCache) {
    CHECK(subgraphVersionCache != nullptr);
    std::vector<Version> operandVersions(operands.size(), Version::ANDROID_OC_MR1);
    for (uint32_t index : operation.inputs) {
        NN_VALIDATE_LT(index, operands.size());
        const Operand& operand = operands[index];
        operandVersions[index] = NN_TRY(validateOperandAndAnythingItDependsOn(
                operand, operandValuesSize, poolSizes, subgraphs, subgraphVersionCache));
    }
    for (uint32_t index : operation.outputs) {
        NN_VALIDATE_LT(index, operands.size());
        const Operand& operand = operands[index];
        operandVersions[index] = NN_TRY(validateOperandAndAnythingItDependsOn(
                operand, operandValuesSize, poolSizes, subgraphs, subgraphVersionCache));
    }
    return validateOperationIncludingOperandVersions(operation, operands, operandVersions,
                                                     subgraphs);
}

Result<Version> validateOperandAndAnythingItDependsOn(const Operand& operand,
                                                      size_t operandValuesSize,
                                                      const std::vector<size_t>& poolSizes,
                                                      const std::vector<Model::Subgraph>& subgraphs,
                                                      SubgraphVersionCache* subgraphVersionCache) {
    CHECK(subgraphVersionCache != nullptr);
    return validateOperand(operand, operandValuesSize, poolSizes, subgraphs,
                           &subgraphVersionCache->cache);
}

}  // namespace android::nn
