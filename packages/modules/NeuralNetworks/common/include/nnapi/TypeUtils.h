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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_TYPE_UTILS_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_TYPE_UTILS_H

#include <android-base/logging.h>
#include <android-base/macros.h>

#include <ostream>
#include <utility>
#include <vector>

#include "nnapi/OperandTypes.h"
#include "nnapi/OperationTypes.h"
#include "nnapi/Result.h"
#include "nnapi/Types.h"

namespace android::nn {

enum class HalVersion : int32_t {
    UNKNOWN,
    V1_0,
    V1_1,
    V1_2,
    V1_3,
    AIDL_UNSTABLE,
    LATEST = V1_3,
};

bool isExtension(OperandType type);
bool isExtension(OperationType type);

bool isNonExtensionScalar(OperandType operandType);

size_t getNonExtensionSize(OperandType operandType);

inline uint16_t getExtensionPrefix(uint32_t type) {
    return static_cast<uint16_t>(type >> kExtensionTypeBits);
}

inline uint16_t getTypeWithinExtension(uint32_t type) {
    return static_cast<uint16_t>(type & kTypeWithinExtensionMask);
}

std::optional<size_t> getNonExtensionSize(OperandType operandType, const Dimensions& dimensions);
std::optional<size_t> getNonExtensionSize(const Operand& operand);

size_t getOffsetFromInts(int lower, int higher);
std::pair<int32_t, int32_t> getIntsFromOffset(size_t offset);

Result<std::vector<uint32_t>> countNumberOfConsumers(size_t numberOfOperands,
                                                     const std::vector<nn::Operation>& operations);

// Combine two tensor dimensions, both may have unspecified dimensions or rank.
Result<Dimensions> combineDimensions(const Dimensions& lhs, const Dimensions& rhs);

// Returns the operandValues's size and a size for each pool in the provided model.
std::pair<size_t, std::vector<size_t>> getMemorySizes(const Model& model);

// Round up "size" to the nearest multiple of "multiple". "multiple" must be a power of 2.
size_t roundUp(size_t size, size_t multiple);

// Returns the alignment for data of the specified length.  It aligns object of length:
// 2, 3 on a 2 byte boundary,
// 4+ on a 4 byte boundary.
// We may want to have different alignments for tensors.
// TODO: This is arbitrary, more a proof of concept.  We need to determine what this should be.
//
// Note that Types.cpp ensures `new` has sufficient alignment for all alignments returned by this
// function. If this function is changed to return different alignments (e.g., 8 byte boundary
// alignment), the code check in Types.cpp similarly needs to be updated.
size_t getAlignmentForLength(size_t length);

// Set of output utility functions.
std::ostream& operator<<(std::ostream& os, const DeviceStatus& deviceStatus);
std::ostream& operator<<(std::ostream& os, const ExecutionPreference& executionPreference);
std::ostream& operator<<(std::ostream& os, const DeviceType& deviceType);
std::ostream& operator<<(std::ostream& os, const MeasureTiming& measureTiming);
std::ostream& operator<<(std::ostream& os, const OperandType& operandType);
std::ostream& operator<<(std::ostream& os, const Operand::LifeTime& lifetime);
std::ostream& operator<<(std::ostream& os, const OperationType& operationType);
std::ostream& operator<<(std::ostream& os, const Request::Argument::LifeTime& lifetime);
std::ostream& operator<<(std::ostream& os, const Priority& priority);
std::ostream& operator<<(std::ostream& os, const ErrorStatus& errorStatus);
std::ostream& operator<<(std::ostream& os, const FusedActivationFunc& activation);
std::ostream& operator<<(std::ostream& os, const OutputShape& outputShape);
std::ostream& operator<<(std::ostream& os, const Timing& timing);
std::ostream& operator<<(std::ostream& os, const Capabilities::PerformanceInfo& performanceInfo);
std::ostream& operator<<(std::ostream& os,
                         const Capabilities::OperandPerformance& operandPerformance);
std::ostream& operator<<(std::ostream& os,
                         const Capabilities::OperandPerformanceTable& operandPerformances);
std::ostream& operator<<(std::ostream& os, const Capabilities& capabilities);
std::ostream& operator<<(std::ostream& os,
                         const Extension::OperandTypeInformation& operandTypeInformation);
std::ostream& operator<<(std::ostream& os, const Extension& extension);
std::ostream& operator<<(std::ostream& os, const DataLocation& location);
std::ostream& operator<<(std::ostream& os,
                         const Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams);
std::ostream& operator<<(std::ostream& os, const Operand::ExtraParams& extraParams);
std::ostream& operator<<(std::ostream& os, const Operand& operand);
std::ostream& operator<<(std::ostream& os, const Operation& operation);
std::ostream& operator<<(std::ostream& os, const SharedHandle& handle);
std::ostream& operator<<(std::ostream& os, const Memory& memory);
std::ostream& operator<<(std::ostream& os, const SharedMemory& memory);
std::ostream& operator<<(std::ostream& os, const MemoryPreference& memoryPreference);
std::ostream& operator<<(std::ostream& os, const Model::Subgraph& subgraph);
std::ostream& operator<<(std::ostream& os, const Model::OperandValues& operandValues);
std::ostream& operator<<(std::ostream& os,
                         const Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
std::ostream& operator<<(std::ostream& os, const Model& model);
std::ostream& operator<<(std::ostream& os, const BufferDesc& bufferDesc);
std::ostream& operator<<(std::ostream& os, const BufferRole& bufferRole);
std::ostream& operator<<(std::ostream& os, const Request::Argument& requestArgument);
std::ostream& operator<<(std::ostream& os, const Request::MemoryPool& memoryPool);
std::ostream& operator<<(std::ostream& os, const Request& request);
std::ostream& operator<<(std::ostream& os, const SyncFence::FenceState& fenceState);
std::ostream& operator<<(std::ostream& os, const TimePoint& timePoint);
std::ostream& operator<<(std::ostream& os, const OptionalTimePoint& optionalTimePoint);
std::ostream& operator<<(std::ostream& os, const Duration& timeoutDuration);
std::ostream& operator<<(std::ostream& os, const OptionalDuration& optionalTimeoutDuration);
std::ostream& operator<<(std::ostream& os, const Version& version);
std::ostream& operator<<(std::ostream& os, const HalVersion& halVersion);

bool operator==(const Timing& a, const Timing& b);
bool operator!=(const Timing& a, const Timing& b);
bool operator==(const Capabilities::PerformanceInfo& a, const Capabilities::PerformanceInfo& b);
bool operator!=(const Capabilities::PerformanceInfo& a, const Capabilities::PerformanceInfo& b);
bool operator==(const Capabilities::OperandPerformance& a,
                const Capabilities::OperandPerformance& b);
bool operator!=(const Capabilities::OperandPerformance& a,
                const Capabilities::OperandPerformance& b);
bool operator==(const Capabilities& a, const Capabilities& b);
bool operator!=(const Capabilities& a, const Capabilities& b);
bool operator==(const Extension::OperandTypeInformation& a,
                const Extension::OperandTypeInformation& b);
bool operator!=(const Extension::OperandTypeInformation& a,
                const Extension::OperandTypeInformation& b);
bool operator==(const Extension& a, const Extension& b);
bool operator!=(const Extension& a, const Extension& b);
bool operator==(const MemoryPreference& a, const MemoryPreference& b);
bool operator!=(const MemoryPreference& a, const MemoryPreference& b);
bool operator==(const Operand::SymmPerChannelQuantParams& a,
                const Operand::SymmPerChannelQuantParams& b);
bool operator!=(const Operand::SymmPerChannelQuantParams& a,
                const Operand::SymmPerChannelQuantParams& b);
bool operator==(const Operand& a, const Operand& b);
bool operator!=(const Operand& a, const Operand& b);
bool operator==(const Operation& a, const Operation& b);
bool operator!=(const Operation& a, const Operation& b);

// The NN_RET_CHECK family of macros defined below is similar to the CHECK family defined in
// system/libbase/include/android-base/logging.h
//
// The difference is that NN_RET_CHECK macros use LOG(ERROR) instead of LOG(FATAL)
// and return false instead of aborting.

// Logs an error and returns false. Append context using << after. For example:
//
//   NN_RET_CHECK_FAIL() << "Something went wrong";
//
// The containing function must return a bool.
#define NN_RET_CHECK_FAIL()                   \
    return ::android::nn::FalseyErrorStream() \
           << "NN_RET_CHECK failed (" << __FILE__ << ":" << __LINE__ << "): "

// Logs an error and returns false if condition is false. Extra logging can be appended using <<
// after. For example:
//
//   NN_RET_CHECK(false) << "Something went wrong";
//
// The containing function must return a bool.
#define NN_RET_CHECK(condition) \
    while (UNLIKELY(!(condition))) NN_RET_CHECK_FAIL() << #condition << " "

// Helper for NN_CHECK_xx(x, y) macros.
#define NN_RET_CHECK_OP(LHS, RHS, OP)                                                       \
    for (auto _values = ::android::base::MakeEagerEvaluator(LHS, RHS);                      \
         UNLIKELY(!(_values.lhs.v OP _values.rhs.v));                                       \
         /* empty */)                                                                       \
    NN_RET_CHECK_FAIL()                                                                     \
            << #LHS << " " << #OP << " " << #RHS << " (" << #LHS << " = "                   \
            << ::android::base::LogNullGuard<decltype(_values.lhs.v)>::Guard(_values.lhs.v) \
            << ", " << #RHS << " = "                                                        \
            << ::android::base::LogNullGuard<decltype(_values.rhs.v)>::Guard(_values.rhs.v) \
            << ") "

// Logs an error and returns false if a condition between x and y does not hold. Extra logging can
// be appended using << after. For example:
//
//   NN_RET_CHECK_EQ(a, b) << "Something went wrong";
//
// The values must implement the appropriate comparison operator as well as
// `operator<<(std::ostream&, ...)`.
// The containing function must return a bool.
#define NN_RET_CHECK_EQ(x, y) NN_RET_CHECK_OP(x, y, ==)
#define NN_RET_CHECK_NE(x, y) NN_RET_CHECK_OP(x, y, !=)
#define NN_RET_CHECK_LE(x, y) NN_RET_CHECK_OP(x, y, <=)
#define NN_RET_CHECK_LT(x, y) NN_RET_CHECK_OP(x, y, <)
#define NN_RET_CHECK_GE(x, y) NN_RET_CHECK_OP(x, y, >=)
#define NN_RET_CHECK_GT(x, y) NN_RET_CHECK_OP(x, y, >)

// Ensure that every user of FalseyErrorStream is linked to the
// correct instance, using the correct LOG_TAG
namespace {

// A wrapper around LOG(ERROR) that can be implicitly converted to bool (always evaluates to false).
// Used to implement stream logging in NN_RET_CHECK.
class FalseyErrorStream {
    DISALLOW_COPY_AND_ASSIGN(FalseyErrorStream);

   public:
    FalseyErrorStream() {}

    template <typename T>
    FalseyErrorStream& operator<<(const T& value) {
        mBuffer << value;
        return *this;
    }

    ~FalseyErrorStream() { LOG(ERROR) << mBuffer.str(); }

    operator bool() const { return false; }

    operator Result<Version>() const { return error() << mBuffer.str(); }

   private:
    std::ostringstream mBuffer;
};

}  // namespace

}  // namespace android::nn

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_TYPE_UTILS_H
