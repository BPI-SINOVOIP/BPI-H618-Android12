/*
 * Copyright (C) 2018 ARM Limited. All rights reserved.
 *
 * Copyright 2016 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_x_GRALLOC_MAPPER_H
#define ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_x_GRALLOC_MAPPER_H

#if HIDL_MAPPER_VERSION_SCALED == 200
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#elif HIDL_MAPPER_VERSION_SCALED == 210
#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#endif

#include "mali_gralloc_module.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace HIDL_IMAPPER_NAMESPACE {
namespace implementation {

using V2_0::Error;
using V2_0::BufferDescriptor;
using V2_0::YCbCrLayout;

class GrallocMapper : public IMapper
{
public:
	GrallocMapper();
	~GrallocMapper();

	/* Override the public IMapper 2.0 interface */

	/* Creates a buffer descriptor */
	Return<void> createDescriptor(const V2_0::IMapper::BufferDescriptorInfo& descriptorInfo,
	                              createDescriptor_cb hidl_cb) override;

	/* Imports a raw buffer handle to create an imported buffer handle */
	Return<void> importBuffer(const hidl_handle& rawHandle,
	                          importBuffer_cb hidl_cb) override;

	/* Frees a buffer handle */
	Return<Error> freeBuffer(void* buffer) override;

	/* Locks the given buffer for the specified CPU usage */
	Return<void> lock(void* buffer, uint64_t cpuUsage,
	                  const IMapper::Rect& accessRegion,
	                  const hidl_handle& acquireFence,
	                  lock_cb hidl_cb) override;

	/*
	 * Locks the given buffer for the specified CPU usage and exports CPU
	 * accessible data in YCbCr structure
	 */
	Return<void> lockYCbCr(void* buffer, uint64_t cpuUsage,
	                       const IMapper::Rect& accessRegion,
	                       const hidl_handle& acquireFence,
	                       lockYCbCr_cb hidl_cb) override;

	/* Unlocks a buffer to indicate all CPU accesses to the buffer have completed */
	Return<void> unlock(void* buffer, unlock_cb hidl_cb) override;

#if HIDL_MAPPER_VERSION_SCALED >= 210
	/* Override the public IMapper 2.1 specific interface */

	/* Validates the buffer against specified descriptor attributes */
	Return<Error> validateBufferSize(void* buffer,
	                                 const V2_1::IMapper::BufferDescriptorInfo& descriptorInfo,
	                                 uint32_t stride) override;

	/* Get the transport size of a buffer */
	Return<void> getTransportSize(void* buffer, getTransportSize_cb _hidl_cb) override;

	/* Creates a descriptor using IMapper 2.1 version of descriptor attributes */
	Return<void> createDescriptor_2_1(const V2_1::IMapper::BufferDescriptorInfo& descriptorInfo,
	                                  createDescriptor_2_1_cb _hidl_cb) override;
#endif

private:
	/* Validates incoming IMapper descriptor attributes */
	bool validateDescriptorInfo(void *descriptor_attr) const;

	/* Register a buffer.  The handle is already cloned by the caller */
	Error registerBuffer(buffer_handle_t bufferHandle) const;

	/* Unmap a buffer.  The handle is already cloned by the caller */
	Error unregisterBuffer(buffer_handle_t bufferHandle) const;

	/* Lock a buffer.  The fence is owned by the caller */
	Error lockBuffer(buffer_handle_t bufferHandle, uint64_t cpuUsage,
	                 const IMapper::Rect& accessRegion, int fenceFd,
	                 void** outData) const;

	/* Lock a buffer, with YCbCr data exported.  The fence is owned by the caller */
	Error lockBuffer(buffer_handle_t bufferHandle, uint64_t cpuUsage,
	                 const IMapper::Rect& accessRegion, int fenceFd,
	                 YCbCrLayout* outLayout) const;

	/* Unlock a buffer.  The returned fence is owned by the caller */
	Error unlockBuffer(buffer_handle_t bufferHandle,
	                   int* outFenceFd) const;

	/* Retrieves the file descriptor referring to a sync fence object */
	bool getFenceFd(const hidl_handle& fenceHandle, int* outFenceFd) const;

	/* Populates the HIDL fence handle for the given fence object */
	hidl_handle getFenceHandle(int fenceFd, char* handleStorage) const;

	/* To hold state information for the instance of mapper */
	struct private_module_t privateModule;
};

extern "C" IMapper* HIDL_FETCH_IMapper(const char* name);

} // namespace implementation
} // namespace HIDL_IMAPPER_NAMESPACE
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android

#endif // ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_x_GRALLOC_MAPPER_H
