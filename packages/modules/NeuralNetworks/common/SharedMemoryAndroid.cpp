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

#define LOG_TAG "SharedMemoryAndroid"

#include <android-base/logging.h>
#include <android-base/mapped_file.h>
#include <android-base/scopeguard.h>
#include <android/hardware_buffer.h>

#include <algorithm>
#include <any>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Result.h"
#include "SharedMemory.h"
#include "TypeUtils.h"
#include "Types.h"

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <hidl/HidlSupport.h>
#include <hidlmemory/mapping.h>
#include <sys/mman.h>
#else
#include "DynamicCLDeps.h"
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

namespace android::nn {
namespace {

GeneralResult<SharedMemory> createSharedMemoryFromUniqueFd(size_t size, int prot,
                                                           base::unique_fd fd, size_t offset) {
    auto handle = Memory::Fd{
            .size = size,
            .prot = prot,
            .fd = std::move(fd),
            .offset = offset,
    };
    return std::make_shared<const Memory>(Memory{.handle = std::move(handle)});
}

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD

using ::android::hardware::hidl_memory;
using ::android::hidl::allocator::V1_0::IAllocator;

const char* const kAllocatorService = "ashmem";

GeneralResult<hardware::hidl_handle> hidlHandleFromUniqueFd(base::unique_fd fd) {
    native_handle_t* nativeHandle = native_handle_create(1, 0);
    if (nativeHandle == nullptr) {
        return NN_ERROR() << "Failed to create native_handle";
    }
    nativeHandle->data[0] = fd.release();

    hardware::hidl_handle hidlHandle;
    hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    return hidlHandle;
}

GeneralResult<SharedMemory> allocateSharedMemory(size_t size) {
    static const auto allocator = IAllocator::getService(kAllocatorService);
    CHECK_GT(size, 0u);

    hidl_memory maybeMemory;
    auto fn = [&maybeMemory](bool success, const hidl_memory& memory) {
        if (success) {
            maybeMemory = memory;
        }
    };
    allocator->allocate(size, fn);

    if (!maybeMemory.valid()) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "IAllocator::allocate returned an invalid (empty) memory object";
    }
    if (maybeMemory.handle()->numFds != 1) {
        return NN_ERROR() << "IAllocator::allocate returned an invalid memory object with "
                          << maybeMemory.handle()->numFds << " numFds, but expected 1";
    }
    if (maybeMemory.handle()->numInts != 0) {
        return NN_ERROR() << "IAllocator::allocate returned an invalid memory object with "
                          << maybeMemory.handle()->numInts << " numInts, but expected 0";
    }

    CHECK_LE(maybeMemory.size(), std::numeric_limits<size_t>::max());
    const int fd = maybeMemory.handle()->data[0];

    auto handle = Memory::Ashmem{
            .fd = NN_TRY(dupFd(fd)),
            .size = static_cast<size_t>(maybeMemory.size()),
    };
    return std::make_shared<const Memory>(Memory{.handle = std::move(handle)});
}

GeneralResult<Mapping> map(const Memory::Ashmem& memory) {
    auto handle = NN_TRY(hidlHandleFromUniqueFd(NN_TRY(dupFd(memory.fd))));
    const auto hidlMemory = hidl_memory("ashmem", std::move(handle), memory.size);

    const auto mapping = mapMemory(hidlMemory);
    if (mapping == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to map memory";
    }

    auto* const pointer = mapping->getPointer().withDefault(nullptr);
    if (pointer == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to get the mapped pointer";
    }

    const auto fullSize = mapping->getSize().withDefault(0);
    if (fullSize == 0 || fullSize > std::numeric_limits<size_t>::max()) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to get the mapped size";
    }

    const size_t size = static_cast<size_t>(fullSize);

    return Mapping{
            .pointer = pointer,
            .size = size,
            .context = mapping,
    };
}

#else

GeneralResult<SharedMemory> allocateSharedMemory(size_t size) {
    CHECK_GT(size, 0u);

    const CompatibilityLayerMemory& memory = loadCompatibilityLayerMemory();
    auto fd = base::unique_fd(memory.create(nullptr, size));
    if (!fd.ok()) {
        return NN_ERROR() << "ASharedMemory_create failed";
    }

    const size_t readSize = memory.getSize(fd.get());
    CHECK_GE(readSize, size);

    constexpr int prot = PROT_READ | PROT_WRITE;
    constexpr size_t offset = 0;
    return createSharedMemoryFromUniqueFd(size, prot, std::move(fd), offset);
}

GeneralResult<Mapping> map(const Memory::Ashmem& /*memory*/) {
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map ashmem memory";
}

#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

size_t getSize(const Memory::Ashmem& memory) {
    return memory.size;
}

size_t getSize(const Memory::Fd& memory) {
    return memory.size;
}

size_t getSize(const Memory::HardwareBuffer& memory) {
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(memory.handle.get(), &desc);
    return desc.format == AHARDWAREBUFFER_FORMAT_BLOB ? desc.width : 0;
}

size_t getSize(const Memory::Unknown& memory) {
    return memory.size;
}

struct MmapFdMappingContext {
    int prot;
    std::any context;
};

GeneralResult<Mapping> map(const Memory::Fd& memory) {
    std::shared_ptr<base::MappedFile> mapping =
            base::MappedFile::FromFd(memory.fd, memory.offset, memory.size, memory.prot);
    if (mapping == nullptr) {
        return NN_ERROR() << "Can't mmap the file descriptor.";
    }
    char* data = mapping->data();

    const bool writable = (memory.prot & PROT_WRITE) != 0;
    std::variant<const void*, void*> pointer;
    if (writable) {
        pointer = static_cast<void*>(data);
    } else {
        pointer = static_cast<const void*>(data);
    }

    auto context = MmapFdMappingContext{.prot = memory.prot, .context = std::move(mapping)};
    return Mapping{.pointer = pointer, .size = memory.size, .context = std::move(context)};
}

GeneralResult<Mapping> map(const Memory::HardwareBuffer& memory) {
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(memory.handle.get(), &desc);

    if (desc.format != AHARDWAREBUFFER_FORMAT_BLOB) {
        return NN_ERROR() << "Unable to map non-blob AHardwareBuffer memory";
    }
    const uint32_t size = desc.width;

    const uint64_t kCpuUsageMask =
            AHARDWAREBUFFER_USAGE_CPU_READ_MASK | AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK;
    void* data = nullptr;
    const auto status = AHardwareBuffer_lock(memory.handle.get(), desc.usage & kCpuUsageMask, -1,
                                             nullptr, &data);
    if (status != /*NO_ERROR*/ 0) {
        return NN_ERROR() << "Can't lock the AHardwareBuffer. Error: " << status;
    }

    // Create shared scoped object to munmap.
    auto scoped = base::make_scope_guard(
            [ahwb = memory.handle.get()] { AHardwareBuffer_unlock(ahwb, nullptr); });
    auto sharedScoped = std::make_shared<decltype(scoped)>(std::move(scoped));

    return Mapping{.pointer = data, .size = size, .context = std::move(sharedScoped)};
}

GeneralResult<Mapping> map(const Memory::Unknown& /*memory*/) {
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map Unknown memory";
}

void freeHardwareBuffer(AHardwareBuffer* buffer) {
    if (buffer) {
        AHardwareBuffer_release(buffer);
    }
}

void freeNoop(AHardwareBuffer* /*buffer*/) {}

}  // namespace

GeneralResult<base::unique_fd> dupFd(int fd) {
    if (fd < 0) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "dupFd was passed an invalid fd";
    }
    auto uniqueFd = base::unique_fd(dup(fd));
    if (!uniqueFd.ok()) {
        // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return here?
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
    }
    return uniqueFd;
}

GeneralResult<SharedMemory> createSharedMemory(size_t size) {
    return allocateSharedMemory(size);
}

GeneralResult<SharedMemory> createSharedMemoryFromFd(size_t size, int prot, int fd, size_t offset) {
    return createSharedMemoryFromUniqueFd(size, prot, NN_TRY(dupFd(fd)), offset);
}

GeneralResult<SharedMemory> createSharedMemoryFromAHWB(AHardwareBuffer* ahwb, bool takeOwnership) {
    CHECK(ahwb != nullptr);
    const Memory::HardwareBuffer::Deleter deleter = (takeOwnership ? freeHardwareBuffer : freeNoop);
    Memory::HardwareBuffer handle = {.handle = Memory::HardwareBuffer::Handle(ahwb, deleter)};
    return std::make_shared<const Memory>(Memory{.handle = std::move(handle)});
}

size_t getSize(const SharedMemory& memory) {
    CHECK(memory != nullptr);
    return std::visit([](const auto& x) { return getSize(x); }, memory->handle);
}

bool isAhwbBlob(const Memory::HardwareBuffer& memory) {
    AHardwareBuffer* ahwb = memory.handle.get();
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(ahwb, &desc);
    return desc.format == AHARDWAREBUFFER_FORMAT_BLOB;
}

bool isAhwbBlob(const SharedMemory& memory) {
    CHECK(memory != nullptr);
    if (!std::holds_alternative<Memory::HardwareBuffer>(memory->handle)) {
        return false;
    }
    return isAhwbBlob(std::get<Memory::HardwareBuffer>(memory->handle));
}

GeneralResult<Mapping> map(const SharedMemory& memory) {
    if (memory == nullptr) {
        return NN_ERROR() << "Unable to map nullptr SharedMemory object";
    }
    return std::visit([](const auto& x) { return map(x); }, memory->handle);
}

bool flush(const Mapping& mapping) {
    if (const auto* mmapFdMapping = std::any_cast<MmapFdMappingContext>(&mapping.context)) {
        if (!std::holds_alternative<void*>(mapping.pointer)) {
            return true;
        }
        void* data = std::get<void*>(mapping.pointer);
        const int prot = mmapFdMapping->prot;
        if (prot & PROT_WRITE) {
            const size_t size = mapping.size;
            return msync(data, size, MS_SYNC) == 0;
        }
    }
    // No-op for other types of memory.
    return true;
}

}  // namespace android::nn
