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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_SHARED_MEMORY_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_SHARED_MEMORY_H

#include <android-base/unique_fd.h>

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "nnapi/Result.h"
#include "nnapi/Types.h"

namespace android::nn {

class MutableMemoryBuilder {
   public:
    explicit MutableMemoryBuilder(uint32_t poolIndex);

    DataLocation append(size_t length, size_t alignment = kMinMemoryAlignment,
                        size_t padding = kMinMemoryPadding);
    bool empty() const;

    GeneralResult<SharedMemory> finish();

   private:
    uint32_t mPoolIndex;
    size_t mSize = 0;
};

class ConstantMemoryBuilder {
   public:
    explicit ConstantMemoryBuilder(uint32_t poolIndex);

    DataLocation append(const void* data, size_t length);
    bool empty() const;

    GeneralResult<SharedMemory> finish();

   private:
    struct LazyCopy {
        const void* data;
        size_t length;
        size_t offset;
    };

    MutableMemoryBuilder mBuilder;
    std::vector<LazyCopy> mSlices;
};

GeneralResult<base::unique_fd> dupFd(int fd);

// Precondition: `*ForwardFdIt` must be convertible to `int`
template <typename ForwardFdIt>
GeneralResult<std::vector<base::unique_fd>> dupFds(ForwardFdIt first, ForwardFdIt last) {
    std::vector<base::unique_fd> fds;
    fds.reserve(std::distance(first, last));
    for (; first != last; ++first) {
        const int fd = *first;
        fds.push_back(NN_TRY(dupFd(fd)));
    }
    return fds;
}

// Precondition: size > 0
GeneralResult<SharedMemory> createSharedMemory(size_t size);

// Duplicates `fd` and takes ownership of the duplicate.
// Precondition: size > 0
GeneralResult<SharedMemory> createSharedMemoryFromFd(size_t size, int prot, int fd, size_t offset);

// Precondition: ahwb != nullptr
GeneralResult<SharedMemory> createSharedMemoryFromAHWB(AHardwareBuffer* ahwb, bool takeOwnership);

// Precondition: memory != nullptr
size_t getSize(const SharedMemory& memory);

bool isAhwbBlob(const Memory::HardwareBuffer& memory);

// Precondition: memory != nullptr
bool isAhwbBlob(const SharedMemory& memory);

struct Mapping {
    std::variant<const void*, void*> pointer;
    size_t size;
    std::any context;
};

GeneralResult<Mapping> map(const SharedMemory& memory);

bool flush(const Mapping& mapping);

}  // namespace android::nn

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_SHARED_MEMORY_H
