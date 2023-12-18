/*
 * Copyright (C) 2018-2019 ARM Limited. All rights reserved.
 *
 * Copyright 2016 The Android Open Source Project
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

#include <inttypes.h>
#include <log/log.h>
#include <sync/sync.h>
#include <mutex>
#include <unordered_set>
#include <system/window.h>

#include "GrallocMapper.h"
#include "GrallocBufferDescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_bufferaccess.h"
#include "mali_gralloc_reference.h"
#include "mali_gralloc_ion.h"
#include "mali_gralloc_buffer.h"
#include "format_info.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace HIDL_IMAPPER_NAMESPACE {
namespace implementation {

/* Default definitions for IMapper 2.x interface */
using V2_0::Error;
using V2_0::BufferDescriptor;

namespace {

/* An unordered set to internally store / retrieve imported buffer handles */
class RegisteredHandlePool
{
public:
	/* Stores the buffer handle in the internal list */
	bool add(buffer_handle_t bufferHandle)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return bufPool.insert(bufferHandle).second;
	}

	/* Retrieves and removes the buffer handle from internal list */
	native_handle_t* remove(void* buffer)
	{
		auto bufferHandle = static_cast<native_handle_t*>(buffer);

		std::lock_guard<std::mutex> lock(mutex);
		return bufPool.erase(bufferHandle) == 1 ? bufferHandle : nullptr;
	}

	/* Retrieves the buffer handle from internal list */
	buffer_handle_t get(const void* buffer)
	{
		auto bufferHandle = static_cast<buffer_handle_t>(buffer);

		std::lock_guard<std::mutex> lock(mutex);
		return bufPool.count(bufferHandle) == 1 ? bufferHandle : nullptr;
	}

private:
	std::mutex mutex;
	std::unordered_set<buffer_handle_t> bufPool;
};

/* GraphicBufferMapper is expected to be valid (and leaked) during process
 * termination. IMapper, and in turn, gRegisteredHandles must be valid as
 * well. Create the registered handle pool on the heap, and let
 * it leak for simplicity.
 *
 * However, there is no way to make sure gralloc0/gralloc1 are valid. Any use
 * of static/global object in gralloc0/gralloc1 that may have been destructed
 * is potentially broken.
 */
RegisteredHandlePool* gRegisteredHandles = new RegisteredHandlePool;

}  /* anonymous namespace */

/**
 *  IMapper constructor. All the state information required for the Gralloc
 *  private module is populated in its default constructor. Gralloc 2.0 specific
 *  state information can be populated here.
 *
 * @return None
 */
GrallocMapper::GrallocMapper()
{
}

/*
 *  IMapper destructor. All the resources aquired for Gralloc private module
 *  (in the IMapper context) are released
 *
 * @return None
 */
GrallocMapper::~GrallocMapper()
{
	mali_gralloc_ion_close();
}

/*
 * Validates incoming IMapper descriptor attributes
 *
 * @param descriptor_attr [in]  Specifies the attributes of the descriptor.
 *
 * @return false, for invalid buffer attributes
 *         true, otherwise
 * Note:   Latest IMapper version encapsulates all previous versions, hence all
 *         incoming descriptor information is mapped to the latest IMapper for validation
 */
bool GrallocMapper::validateDescriptorInfo(void *descriptor_attr) const
{
	BufferDescriptorInfo * const descriptorInfo = (BufferDescriptorInfo *)descriptor_attr;

	using android::hardware::graphics::common::HIDL_COMMON_NAMESPACE::BufferUsage;
	using android::hardware::graphics::common::HIDL_COMMON_NAMESPACE::PixelFormat;

	const uint64_t validUsageBits =
#if HIDL_MAPPER_VERSION_SCALED >= 210
		BufferUsage::GPU_CUBE_MAP |
		BufferUsage::GPU_MIPMAP_COMPLETE |
#endif
		BufferUsage::CPU_READ_MASK | BufferUsage::CPU_WRITE_MASK |
		BufferUsage::GPU_TEXTURE | BufferUsage::GPU_RENDER_TARGET |
		BufferUsage::COMPOSER_OVERLAY | BufferUsage::COMPOSER_CLIENT_TARGET |
		BufferUsage::CAMERA_INPUT | BufferUsage::CAMERA_OUTPUT |
		BufferUsage::PROTECTED |
		BufferUsage::COMPOSER_CURSOR |
		BufferUsage::VIDEO_ENCODER |
		BufferUsage::RENDERSCRIPT |
		BufferUsage::VIDEO_DECODER |
		BufferUsage::SENSOR_DIRECT_DATA |
		BufferUsage::GPU_DATA_BUFFER |
		BufferUsage::VENDOR_MASK;

	if (!descriptorInfo->width || !descriptorInfo->height ||
	    !descriptorInfo->layerCount)
	{
		AERR("Invalid buffer descriptor (2.x mapper) attributes, width = %d height = %d  layerCount = %d ",
		      descriptorInfo->width, descriptorInfo->height, descriptorInfo->layerCount);
		return false;
	}

	if (descriptorInfo->format == static_cast<PixelFormat>(0))
	{
		return false;
	}

	if (descriptorInfo->usage & ~validUsageBits)
	{
		/* It is possible that application uses private usage bits. */
		ALOGW("Buffer descriptor with invalid usage bits 0x%" PRIx64,
		       descriptorInfo->usage & ~validUsageBits);
	}

	return true;
}

/*
 * Creates a buffer descriptor from incoming descriptor attributes
 *
 * @param descriptorInfo [in]  Specifies the (2.0 IMapper) attributes of
 *                             the descriptor.
 * @param hidl_cb        [in]  HIDL callback function generating -
 *                             error:      NONE upon success. Otherwise,
 *                                         BAD_VALUE when any of the specified attributes are invalid
 *                             descriptor: Newly created buffer descriptor.
 *
 * @return Void
 */
Return<void> GrallocMapper::createDescriptor(
             const V2_0::IMapper::BufferDescriptorInfo& descriptorInfo,
             createDescriptor_cb hidl_cb)
{
	if (validateDescriptorInfo((void *)&descriptorInfo))
	{
		hidl_cb(Error::NONE, grallocEncodeBufferDescriptor(descriptorInfo));
	}
	else
	{
		AERR("Invalid attributes to create descriptor for Mapper 2.0");
		hidl_cb(Error::BAD_VALUE, BufferDescriptor());
	}

	return Void();
}

/*
 * Imports a raw buffer handle to create an imported buffer handle for use with
 * the rest of the mapper or with other in-process libraries.
 *
 * @param rawHandle [in] Raw buffer handle to import.
 * @param hidl_cb   [in] HIDL Callback function to export output information
 * @param hidl_cb   [in]  HIDL callback function generating -
 *                  error  : NONE upon success. Otherwise,
 *                           BAD_BUFFER for an invalid buffer
 *                           NO_RESOURCES when the raw handle cannot be imported
 *                           BAD_VALUE when any of the specified attributes are invalid
 *                  buffer : Imported buffer handle
 *
 * @return Void
 */
Return<void> GrallocMapper::importBuffer(const hidl_handle& rawHandle,
                                         importBuffer_cb hidl_cb)
{
	if (!rawHandle.getNativeHandle())
	{
		AERR("Invalid buffer handle to import");
		hidl_cb(Error::BAD_BUFFER, nullptr);
		return Void();
	}

	native_handle_t* bufferHandle = native_handle_clone(rawHandle.getNativeHandle());
	if (!bufferHandle)
	{
		AERR("Failed to clone buffer handle");
		hidl_cb(Error::NO_RESOURCES, nullptr);
		return Void();
	}

	// clear the invalid mapping address
	private_handle_t *tmphandle = (private_handle_t *)bufferHandle;
	tmphandle->attr_base = MAP_FAILED;

	const Error error = registerBuffer(bufferHandle);
	if (error != Error::NONE)
	{
		native_handle_close(bufferHandle);
		native_handle_delete(bufferHandle);

		hidl_cb(error, nullptr);
		return Void();
	}

	if (gRegisteredHandles->add(bufferHandle) == false)
	{
		/* The newly cloned handle is already registered. This can only happen
		 * when a handle previously registered was native_handle_delete'd instead
		 * of freeBuffer'd.
		 */
		AERR("Handle %p has already been imported; potential fd leaking",
		       bufferHandle);
		unregisterBuffer(bufferHandle);
		native_handle_close(bufferHandle);
		native_handle_delete(bufferHandle);

		hidl_cb(Error::NO_RESOURCES, nullptr);
		return Void();
	}

	hidl_cb(Error::NONE, bufferHandle);
	return Void();
}

/*
 * Frees a buffer handle and releases all the resources associated with it
 *
 * @param buffer [in] Imported buffer to free
 *
 * @return Error::BAD_BUFFER for an invalid buffer / when failed to free the buffer
 *         Error::NONE on successful free
 */
Return<Error> GrallocMapper::freeBuffer(void* buffer)
{
	native_handle_t * const bufferHandle = gRegisteredHandles->remove(buffer);
	if (!bufferHandle)
	{
		AERR("Invalid buffer handle %p to freeBuffer", buffer);
		return Error::BAD_BUFFER;
	}

	const Error status = unregisterBuffer(bufferHandle);
	if (status != Error::NONE)
	{
		return status;
	}

	native_handle_close(bufferHandle);
	native_handle_delete(bufferHandle);

	return Error::NONE;
}

/*
 * Retrieves the file descriptor referring to a sync fence object
 *
 * @param fenceHandle [in]  HIDL fence handle
 * @param outFenceFd  [out] Fence file descriptor. '-1' indicates no fence
 *
 * @return false, for an invalid HIDL fence handle
 *         true, otherwise
 */
bool GrallocMapper::getFenceFd(const hidl_handle& fenceHandle,
                               int* outFenceFd) const
{
	auto const handle = fenceHandle.getNativeHandle();
	if (handle && handle->numFds > 1)
	{
		AERR("Invalid fence handle with %d fds", handle->numFds);
		return false;
	}

	*outFenceFd = (handle && handle->numFds == 1) ? handle->data[0] : -1;
	return true;
}

/*
 * Populates the HIDL fence handle for the given fence object
 *
 * @param fenceFd       [in] Fence file descriptor
 * @param handleStorage [in] HIDL handle storage for fence
 *
 * @return HIDL fence handle
 */
hidl_handle GrallocMapper::getFenceHandle(int fenceFd, char* handleStorage) const
{
	native_handle_t* handle = nullptr;
	if (fenceFd >= 0)
	{
		handle = native_handle_init(handleStorage, 1, 0);
		handle->data[0] = fenceFd;
	}

	return hidl_handle(handle);
}

/*
 * Locks the given buffer for the specified CPU usage.
 *
 * @param buffer       [in] Buffer to lock
 * @param cpuUsage     [in] Specifies one or more CPU usage flags to request
 * @param accessRegion [in] Portion of the buffer that the client intends to access
 * @param acquireFence [in] Handle for aquire fence object
 * @param hidl_cb      [in] HIDL callback function generating -
 *                          error: NONE upon success. Otherwise,
 *                                 BAD_BUFFER for an invalid buffer
 *                                 BAD_VALUE for an invalid input parameters
 *                          data:  CPU-accessible pointer to the buffer data
 *
 * @return Void
 */
Return<void> GrallocMapper::lock(void* buffer, uint64_t cpuUsage,
                                 const IMapper::Rect& accessRegion,
                                 const hidl_handle& acquireFence,
                                 lock_cb hidl_cb)
{
	buffer_handle_t bufferHandle = gRegisteredHandles->get(buffer);
	if (!bufferHandle)
	{
		AERR("Buffer to lock: %p has not been registered with Gralloc", buffer);
		hidl_cb(Error::BAD_BUFFER, nullptr);
		return Void();
	}

	int fenceFd;
	if (!getFenceFd(acquireFence, &fenceFd))
	{
		hidl_cb(Error::BAD_VALUE, nullptr);
		return Void();
	}

	void* data = nullptr;
	const Error error = lockBuffer(bufferHandle, cpuUsage, accessRegion, fenceFd, &data);
	hidl_cb(error, data);
	return Void();
}

/*
 * Locks the given buffer for the specified CPU usage and exports cpu accessible
 * data in YCbCr structure.
 *
 * @param buffer       [in] Buffer to lock.
 * @param cpuUsage     [in] Specifies one or more CPU usage flags to request
 * @param accessRegion [in] Portion of the buffer that the client intends to access.
 * @param acquireFence [in] Handle for aquire fence object
 * @param hidl_cb      [in] HIDL callback function generating -
 *                          error:  NONE upon success. Otherwise,
 *                                  BAD_BUFFER for an invalid buffer
 *                                  BAD_VALUE for an invalid input parameters
 *                          layout: Data layout of the buffer
 *
 * @return Void
 */
Return<void> GrallocMapper::lockYCbCr(void* buffer, uint64_t cpuUsage,
                                      const IMapper::Rect& accessRegion,
                                      const hidl_handle& acquireFence,
                                      lockYCbCr_cb hidl_cb)
{
	YCbCrLayout layout = {};

	buffer_handle_t bufferHandle = gRegisteredHandles->get(buffer);
	if (!bufferHandle)
	{
		AERR("Buffer to lock(YCbCr): %p has not been registered with Gralloc", buffer);
		hidl_cb(Error::BAD_BUFFER, layout);
		return Void();
	}

	int fenceFd;
	if (!getFenceFd(acquireFence, &fenceFd))
	{
		hidl_cb(Error::BAD_VALUE, layout);
		return Void();
	}

	const Error error = lockBuffer(bufferHandle, cpuUsage, accessRegion, fenceFd, &layout);
	hidl_cb(error, layout);
	return Void();
}

/*
 * Unlocks a buffer to indicate all CPU accesses to the buffer have completed
 *
 * @param buffer       [in] Buffer to lock.
 * @param hidl_cb      [in] HIDL callback function generating -
 *                          error:        NONE upon success. Otherwise,
 *                                        BAD_BUFFER for an invalid buffer
 *                          releaseFence: Referrs to a sync fence object
 *
 * @return Void
 */
Return<void> GrallocMapper::unlock(void* buffer, unlock_cb hidl_cb)
{
	buffer_handle_t bufferHandle = gRegisteredHandles->get(buffer);
	if (!bufferHandle)
	{
		AERR("Buffer to unlock: %p has not been registered with Gralloc", buffer);
		hidl_cb(Error::BAD_BUFFER, nullptr);
		return Void();
	}

	int fenceFd;
	const Error error = unlockBuffer(bufferHandle, &fenceFd);
	if (error == Error::NONE)
	{
		NATIVE_HANDLE_DECLARE_STORAGE(fenceStorage, 1, 0);
		hidl_cb(error, getFenceHandle(fenceFd, fenceStorage));

		if (fenceFd >= 0)
		{
			close(fenceFd);
		}
	}
	else
	{
		hidl_cb(error, nullptr);
	}

	return Void();
}

/*
 * Translates the register buffer API into existing gralloc implementation
 *
 * @param bufferHandle [in] Private handle for the buffer to be imported
 *
 * @return Error::BAD_BUFFER for an invalid buffer
 *         Error::NO_RESOURCES when unable to import the given buffer
 *         Error::NONE on successful import
 */
Error GrallocMapper::registerBuffer(buffer_handle_t bufferHandle) const
{
	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer: %p is corrupted", bufferHandle);
		return Error::BAD_BUFFER;
	}

	if (mali_gralloc_reference_retain(&privateModule, bufferHandle) < 0)
	{
		return Error::NO_RESOURCES;
	}

	return Error::NONE;
}

/*
 * Translates the unregister buffer API into existing gralloc implementation
 *
 * @param bufferHandle [in] Private handle for the buffer to be released
 *
 * @return Error::BAD_BUFFER for an invalid buffer / buffers which can't be released
 *         Error::NONE on successful release of the buffer
 */
Error GrallocMapper::unregisterBuffer(buffer_handle_t bufferHandle) const
{
	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer: %p is corrupted", bufferHandle);
		return Error::BAD_BUFFER;
	}

	const int status = mali_gralloc_reference_release(&privateModule, bufferHandle, true);
	if (status != 0)
	{
		AERR("Unable to release buffer:%p", bufferHandle);
		return Error::BAD_BUFFER;
	}

	return Error::NONE;
}

/*
 * Locks the given buffer for the specified CPU usage.
 *
 * @param bufferHandle [in]  Buffer to lock.
 * @param cpuUsage     [in]  Specifies one or more CPU usage flags to request
 * @param accessRegion [in]  Portion of the buffer that the client intends to access.
 * @param fenceFd      [in]  Fence file descriptor
 * @param outData      [out] CPU accessible buffer address
 *
 * @return Error::BAD_BUFFER for an invalid buffer
 *         Error::NO_RESOURCES when unable to duplicate fence
 *         Error::BAD_VALUE when locking fails
 *         Error::NONE on successful buffer lock
 */
Error GrallocMapper::lockBuffer(buffer_handle_t bufferHandle,
                                uint64_t cpuUsage,
                                const IMapper::Rect& accessRegion, int fenceFd,
                                void** outData) const
{
	/* dup fenceFd as it is going to be owned by gralloc. Note that it is
	 * gralloc's responsibility to close it, even on locking errors.
	 */
	if (fenceFd >= 0)
	{
		fenceFd = dup(fenceFd);
		if (fenceFd < 0)
		{
			AERR("Error encountered while duplicating fence file descriptor");
			return Error::NO_RESOURCES;
		}
	}

	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer: %p is corrupted", bufferHandle);
		return Error::BAD_BUFFER;
	}

	void* data = nullptr;
	if (mali_gralloc_lock_async(&privateModule, bufferHandle, cpuUsage,
	                            accessRegion.left, accessRegion.top,
	                            accessRegion.width, accessRegion.height,
	                           &data, fenceFd) < 0)
	{
		return Error::BAD_VALUE;
	}

	*outData = data;
	return Error::NONE;
}

/*
 * Locks the given buffer for the specified CPU usage and exports cpu accessible
 * data in YCbCr structure.
 *
 * @param bufferHandle [in]  Buffer to lock.
 * @param cpuUsage     [in]  Specifies one or more CPU usage flags to request
 * @param accessRegion [in]  Portion of the buffer that the client intends to access.
 * @param fenceFd      [in]  Fence file descriptor
 * @param outLayout    [out] Describes CPU accessible information in YCbCr format
 *
 * @return Error::BAD_BUFFER for an invalid buffer
 *         Error::NO_RESOURCES when unable to duplicate fence
 *         Error::BAD_VALUE when locking fails
 *         Error::NONE on successful buffer lock
 */
Error GrallocMapper::lockBuffer(buffer_handle_t bufferHandle,
                                 uint64_t cpuUsage,
                                 const IMapper::Rect& accessRegion, int fenceFd,
                                 YCbCrLayout* outLayout) const
{
	int result;
	android_ycbcr ycbcr = {};

	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer: %p is corrupted", bufferHandle);
		return Error::BAD_BUFFER;
	}

	if (fenceFd >= 0)
	{
		fenceFd = dup(fenceFd);
		if (fenceFd < 0)
		{
			AERR("Error encountered while duplicating fence file descriptor");
			return Error::NO_RESOURCES;
		}
	}

	result = mali_gralloc_lock_ycbcr_async(&privateModule, bufferHandle, cpuUsage,
	                                       accessRegion.left, accessRegion.top,
	                                       accessRegion.width, accessRegion.height,
	                                       &ycbcr, fenceFd);
	if (result)
	{
		AERR("Locking(YCbCr) failed with error: %d", result);
		return Error::BAD_VALUE;
	}

	outLayout->y = ycbcr.y;
	outLayout->cb = ycbcr.cb;
	outLayout->cr = ycbcr.cr;
	outLayout->yStride = ycbcr.ystride;
	outLayout->cStride = ycbcr.cstride;
	outLayout->chromaStep = ycbcr.chroma_step;

	return Error::NONE;
}

/*
 * Unlocks a buffer to indicate all CPU accesses to the buffer have completed
 *
 * @param bufferHandle [in]  Buffer to lock.
 * @param outFenceFd   [out] Fence file descriptor
 *
 * @return Error::BAD_BUFFER for an invalid buffer
 *         Error::BAD_VALUE when unlocking failed
 *         Error::NONE on successful buffer unlock
 */
Error GrallocMapper::unlockBuffer(buffer_handle_t bufferHandle,
                                  int* outFenceFd) const
{
	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer: %p is corrupted", bufferHandle);
		return Error::BAD_BUFFER;
	}

	int fenceFd = -1;
	const int result = mali_gralloc_unlock_async(&privateModule, bufferHandle, &fenceFd);
	if (result)
	{
		AERR("Unlocking failed with error: %d", result);
		return Error::BAD_VALUE;
	}

	*outFenceFd = fenceFd;

	return Error::NONE;
}

#if HIDL_MAPPER_VERSION_SCALED >= 210

/*
 * Validates the buffer against specified descriptor attributes
 *
 * @param buffer          [in] Buffer which needs to be validated.
 * @param descriptorInfo  [in] Required attributes of the buffer
 * @param in_stride       [in] Buffer stride returned by IAllocator::allocate
 *
 * @return Error::NONE upon success. Otherwise,
 *         Error::BAD_BUFFER upon bad buffer input
 *         Error::BAD_VALUE when any of the specified attributes are invalid
 */
Return<Error> GrallocMapper::validateBufferSize(void* buffer,
                                                const V2_1::IMapper::BufferDescriptorInfo& descriptorInfo,
                                                uint32_t in_stride)
{
	/* The buffer must have been allocated by Gralloc */
	buffer_handle_t bufferHandle = gRegisteredHandles->get(buffer);
	if (!bufferHandle)
	{
		AERR("Buffer: %p has not been registered with Gralloc", buffer);
		return Error::BAD_BUFFER;
	}

	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer: %p is corrupted", bufferHandle);
		return Error::BAD_BUFFER;
	}

	/* All Gralloc allocated buffers must be conform to local descriptor validation */
	if (!validateDescriptorInfo((void *)&descriptorInfo))
	{
		AERR("Invalid descriptor attributes for validating buffer size");
		return Error::BAD_VALUE;
	}

	buffer_descriptor_t grallocDescriptor;
	grallocDescriptor.width = descriptorInfo.width;
	grallocDescriptor.height = descriptorInfo.height;
	grallocDescriptor.layer_count = descriptorInfo.layerCount;
	grallocDescriptor.hal_format = static_cast<uint64_t>(descriptorInfo.format);
	grallocDescriptor.producer_usage = static_cast<uint64_t>(descriptorInfo.usage);
	grallocDescriptor.consumer_usage = grallocDescriptor.producer_usage;
	grallocDescriptor.format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;

	/* Derive the buffer size for the given descriptor */
	const int result = mali_gralloc_derive_format_and_size(&privateModule,
	                                                       &grallocDescriptor);
	if (result)
	{
		AERR("Unable to derive format and size for the given descriptor information. error: %d", result);
		return Error::BAD_VALUE;
	}

	/* Validate the buffer parameters against descriptor info */
	private_handle_t *gralloc_buffer = (private_handle_t *)bufferHandle;

	/* The buffer size must be greater than (or equal to) what would have been allocated with descriptor */
	if ((size_t)gralloc_buffer->size < grallocDescriptor.size)
	{
		ALOGW("Buf size mismatch. Buffer size = %u, Descriptor (derived) size = %zu",
		       gralloc_buffer->size, grallocDescriptor.size);
		return Error::BAD_VALUE;
	}

	if ((uint32_t)gralloc_buffer->stride != in_stride)
	{
		AERR("Stride mismatch. Expected stride = %d, Buffer stride = %d",
		                       in_stride, gralloc_buffer->stride);
		return Error::BAD_VALUE;
	}

	if (gralloc_buffer->internal_format != grallocDescriptor.internal_format)
	{
		AERR("Buffer internal format :0x%" PRIx64" does not match descriptor (derived) internal format :0x%"
		      PRIx64, gralloc_buffer->internal_format, grallocDescriptor.internal_format);
		return Error::BAD_VALUE;
	}

	if (gralloc_buffer->alloc_format != grallocDescriptor.alloc_format)
	{
		AERR("Buffer alloc format :0x%" PRIx64" does not match descriptor (derived) alloc format :0x%"
		      PRIx64, gralloc_buffer->alloc_format, grallocDescriptor.alloc_format);
		return Error::BAD_VALUE;
	}

	const int format_idx = get_format_index(gralloc_buffer->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
	if (format_idx == -1)
	{
		AERR("Invalid format to validate buffer descriptor");
		return Error::BAD_VALUE;
	}
	else
	{
		for (int i = 0; i < formats[format_idx].npln; i++)
		{
			if (gralloc_buffer->plane_info[i].offset != grallocDescriptor.plane_info[i].offset)
			{
				AERR("Buffer offset 0x%x mismatch with desc offset 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].offset, grallocDescriptor.plane_info[i].offset, i);
				return Error::BAD_VALUE;
			}

			if (gralloc_buffer->plane_info[i].byte_stride != grallocDescriptor.plane_info[i].byte_stride)
			{
				AERR("Buffer byte stride 0x%x mismatch with desc byte stride 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].byte_stride, grallocDescriptor.plane_info[i].byte_stride, i);
				return Error::BAD_VALUE;
			}

			if (gralloc_buffer->plane_info[i].alloc_width != grallocDescriptor.plane_info[i].alloc_width)
			{
				AERR("Buffer alloc width 0x%x mismatch with desc alloc width 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].alloc_width, grallocDescriptor.plane_info[i].alloc_width, i);
				return Error::BAD_VALUE;
			}

			if (gralloc_buffer->plane_info[i].alloc_height != grallocDescriptor.plane_info[i].alloc_height)
			{
				AERR("Buffer alloc height 0x%x mismatch with desc alloc height 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].alloc_height, grallocDescriptor.plane_info[i].alloc_height, i);
				return Error::BAD_VALUE;
			}
		}
	}

	if ((uint32_t)gralloc_buffer->width != grallocDescriptor.width)
	{
		AERR("Width mismatch. Buffer width = %u, Descriptor width = %u",
		      gralloc_buffer->width, grallocDescriptor.width);
		return Error::BAD_VALUE;
	}

	if ((uint32_t)gralloc_buffer->height != grallocDescriptor.height)
	{
		AERR("Height mismatch. Buffer height = %u, Descriptor height = %u",
		      gralloc_buffer->height, grallocDescriptor.height);
		return Error::BAD_VALUE;
	}

	if (gralloc_buffer->layer_count != grallocDescriptor.layer_count)
	{
		AERR("Layer Count mismatch. Buffer layer_count = %u, Descriptor layer_count width = %u",
		      gralloc_buffer->layer_count, grallocDescriptor.layer_count);
		return Error::BAD_VALUE;
	}

	return Error::NONE;
}

/*
 * Get the transport size of a buffer
 *
 * @param buffer       [in] Buffer for computing transport size
 * @param hidl_cb      [in] HIDL callback function generating -
 *                          error:   NONE upon success. Otherwise,
 *                                   BAD_BUFFER for an invalid buffer
 *                          numFds:  Number of file descriptors needed for transport
 *                          numInts: Number of integers needed for transport
 *
 * @return Void
 */
Return<void> GrallocMapper::getTransportSize(void* buffer, getTransportSize_cb hidl_cb)
{
	/* The buffer must have been allocated by Gralloc */
	buffer_handle_t bufferHandle = gRegisteredHandles->get(buffer);
	if (!bufferHandle)
	{
		AERR("Buffer %p is not registered with Gralloc", bufferHandle);
		hidl_cb(Error::BAD_BUFFER, -1, -1);
		return Void();
	}

	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Buffer %p is corrupted", buffer);
		hidl_cb(Error::BAD_BUFFER, -1, -1);
		return Void();
	}

	hidl_cb(Error::NONE, GRALLOC_ARM_NUM_FDS, NUM_INTS_IN_PRIVATE_HANDLE);

	return Void();
}

/*
 * Creates a buffer descriptor from incoming descriptor attributes
 *
 * @param descriptorInfo [in]  Specifies the (2.1 IMapper) attributes of
 *                             the descriptor.
 * @param hidl_cb        [in]  HIDL callback function generating -
 *                             error:      NONE upon success. Otherwise,
 *                                         BAD_VALUE when any of the specified attributes are invalid
 *                             descriptor: Newly created buffer descriptor.
 *
 * @return Void
 */
Return<void> GrallocMapper::createDescriptor_2_1(const V2_1::IMapper::BufferDescriptorInfo& descriptorInfo,
                                                 createDescriptor_2_1_cb hidl_cb)
{
	if (validateDescriptorInfo((void *)&descriptorInfo))
	{
		hidl_cb(Error::NONE, grallocEncodeBufferDescriptor(descriptorInfo));
	}
	else
	{
		AERR("Invalid (IMapper 2.1) attributes to create descriptor");
		hidl_cb(Error::BAD_VALUE, V2_0::BufferDescriptor());
	}

	return Void();
}
#endif /* HIDL_MAPPER_VERSION_SCALED >= 210 */

IMapper* HIDL_FETCH_IMapper(const char* /* name */)
{
	ALOGV("Arm Module IMapper %d.%d , pid = %d ppid = %d ", GRALLOC_VERSION_MAJOR,
	       (HIDL_MAPPER_VERSION_SCALED - (GRALLOC_VERSION_MAJOR * 100)) / 10, getpid(), getppid());

	return new GrallocMapper();
}

} // namespace implementation
} // namespace HIDL_IMAPPER_NAMESPACE
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android
